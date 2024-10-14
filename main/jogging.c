// Include esp-idf librarires
#include <math.h>
#include "driver/gpio.h"
#include "esp_log.h"

// Include modules
#include "system.h"
#include "config.h"

static const char *TAG = "Jogging";

void update_velocity(uint32_t target, uint32_t *pos_ptr, float *vel_ptr, float *acc_ptr)
{
    // Based on Unity SmoothDamp() implementation of the critical 
    // Based on Game Programming Gems 4 Chapter 1.10
    float vel = *(vel_ptr);
    uint32_t pos = *(pos_ptr);
    float acc = *acc_ptr;

    float smoothTime = 0.4;
    
    float deltaTime = fabs(1/vel);
    float omega = 2.0f / smoothTime;

    float x = omega * deltaTime;
    float exp = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);
    float change = (float)pos - (float)target; // floats so the rest result is not unsign
    uint32_t originalTo = target;
    float diff = abs((int32_t)pos-(int32_t)originalTo);

    float prev_vel = vel;
    float prev_acc = acc;

    // Clamp maximum speed
    float maxChange = MAX_FEED_RATE * STEPS_PER_MM * smoothTime;
    change = fmax(-maxChange, fmin(change, maxChange));
    target = pos - change;
    
    // calculate velocity with no jerk control
    float temp = (vel + omega * change) * deltaTime;
    vel = (vel - omega * temp) * exp;

    // calculte acceleration and jerk with no jerk control 
    acc = (vel - prev_vel)/deltaTime;
    float jerk = (acc - prev_acc)/deltaTime;
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
    max_jerk = fmin(max_jerk * ((fabs(prev_vel)/maxSpeed2)*(fabs(prev_vel)/maxSpeed2)+0.02), max_jerk);
    jerk = fmax(-max_jerk, fmin(jerk, max_jerk));

    // calculate acceleration and velocity with jerk controlled 
    acc = prev_acc + jerk * deltaTime;
    vel = prev_vel + acc*deltaTime;
    
    // establish minimum velocity to avoid too slow creep in
    if(vel < MIN_FEED_RATE  && vel > 0) {
        vel = MIN_FEED_RATE;
    } else if (vel > -MIN_FEED_RATE && vel < 0) {
        vel = -MIN_FEED_RATE;
    }

    // increment system position
    if (vel > 0) {
        pos += 1;
        gpio_set_level(STEP_MOTOR_GPIO_DIR, STEP_MOTOR_SPIN_DIR_CLOCKWISE);
    } else {
        pos -= 1;
        gpio_set_level(STEP_MOTOR_GPIO_DIR, STEP_MOTOR_SPIN_DIR_COUNTERCLOCKWISE);
    }

    if(vel * prev_vel < 0) {
        if(vel > 0) {
            gpio_set_level(STEP_MOTOR_GPIO_DIR, STEP_MOTOR_SPIN_DIR_CLOCKWISE);
            vTaskDelay(pdMS_TO_TICKS(1000));
        } else {
            gpio_set_level(STEP_MOTOR_GPIO_DIR, STEP_MOTOR_SPIN_DIR_COUNTERCLOCKWISE);
            vTaskDelay(pdMS_TO_TICKS(1000));
        } 
    }

    *(vel_ptr) = vel;
    *(pos_ptr) = pos;
    *(acc_ptr) = acc;

}
