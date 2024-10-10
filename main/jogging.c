// Include esp-idf librarires
#include <math.h>
#include "driver/gpio.h"
#include "esp_log.h"

// Include modules
#include "system.h"
#include "config.h"

static const char *TAG = "Jogging";

void update_velocity(void)
{
    // Based on Unity SmoothDamp() implementation of the critical 
    // Based on Game Programming Gems 4 Chapter 1.10
    float smoothTime = 0.4;
    int32_t target = sys.target.pos;
    float deltaTime = fabs(1/sys.status.vel);
    float omega = 2.0f / smoothTime;

    float x = omega * deltaTime;
    float exp = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);
    float change = (float)sys.status.pos - (float)target; // floats so the rest result is not unsign
    uint32_t originalTo = target;
    float diff = abs((int32_t)sys.status.pos-(int32_t)originalTo);

    sys.prev_status.vel = sys.status.vel;
    sys.prev_status.acc = sys.status.acc;

    // Clamp maximum speed
    float maxChange = MAX_FEED_RATE * STEPS_PER_MM * smoothTime;
    change = fmax(-maxChange, fmin(change, maxChange));
    target = sys.status.pos - change;
    
    // calculate velocity with no jerk control
    float temp = (sys.status.vel + omega * change) * deltaTime;
    sys.status.vel = (sys.status.vel - omega * temp) * exp;

    // calculte acceleration and jerk with no jerk control 
    sys.status.acc = (sys.status.vel - sys.prev_status.vel)/deltaTime;
    float jerk = (sys.status.acc - sys.prev_status.acc)/deltaTime;
    float max_jerk = 4500000;
    float maxSpeed2;

    // variable to make jerk speed dependent, gives a s-curve like shape to the intial portion of the curve
    if (diff < 60000) {
        maxSpeed2 =  5.14712684f/10000000000 * diff * diff * diff -7.61746767f/100000 * diff *diff + 3.65719245f * diff -3.32443566f * 1000 + 3325;
    }
    else {
        maxSpeed2 = MAX_FEED_RATE;
    }

    // calculate jerk
    max_jerk = fmin(max_jerk * ((fabs(sys.prev_status.vel)/maxSpeed2)*(fabs(sys.prev_status.vel)/maxSpeed2)+0.02), max_jerk);
    jerk = fmax(-max_jerk, fmin(jerk, max_jerk));

    // calculate acceleration and velocity with jerk controlled 
    sys.status.acc = sys.prev_status.acc + jerk * deltaTime;
    sys.status.vel = sys.prev_status.vel + sys.status.acc*deltaTime;
    
    // establish minimum velocity to avoid too slow creep in
    if(sys.status.vel < MIN_FEED_RATE  && sys.status.vel > 0) {
        sys.status.vel = MIN_FEED_RATE;
    } else if (sys.status.vel > -MIN_FEED_RATE && sys.status.vel < 0) {
        sys.status.vel = -MIN_FEED_RATE;
    }

    // increment system position
    if (sys.status.vel > 0) {
        sys.status.pos += 1;
        gpio_set_level(STEP_MOTOR_GPIO_DIR, STEP_MOTOR_SPIN_DIR_CLOCKWISE);
    } else {
        sys.status.pos -= 1;
        gpio_set_level(STEP_MOTOR_GPIO_DIR, STEP_MOTOR_SPIN_DIR_COUNTERCLOCKWISE);
    }

    if(sys.status.vel * sys.prev_status.vel < 0) {
        if(sys.status.vel > 0) {
            gpio_set_level(STEP_MOTOR_GPIO_DIR, STEP_MOTOR_SPIN_DIR_CLOCKWISE);
            vTaskDelay(pdMS_TO_TICKS(1000));
        } else {
            gpio_set_level(STEP_MOTOR_GPIO_DIR, STEP_MOTOR_SPIN_DIR_COUNTERCLOCKWISE);
            vTaskDelay(pdMS_TO_TICKS(1000));
        } 
    } 
}
