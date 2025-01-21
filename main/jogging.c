// Include esp-idf librarires
#include <math.h>
#include "driver/gpio.h"
#include "esp_log.h"

// Include modules
#include "system.h"
#include "settings.h"
#include "u_convert.h"

static const char *TAG = "Jogging";
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

void cancel_jog(){
    if(sys.state & STATE_JOGGING) {
        if (jog_aux.status.vel > 0) {
            sys.target.pos = MIN(soft_limits_check(sys.real.pos + (int32_t)settings.motion.jog_cancel_dist), sys.target.pos);
        } else {
            if (sys.real.pos > (int32_t)settings.motion.jog_cancel_dist) {
                sys.target.pos = MAX(soft_limits_check(sys.real.pos - (int32_t)settings.motion.jog_cancel_dist), sys.target.pos);
            }
        }
    }
    return;
}

void update_velocity(uint32_t target, int32_t *pos_ptr, int32_t *vel_ptr)
{
    // Based on Unity SmoothDamp() implementation of the critical 
    // Based on Game Programming Gems 4 Chapter 1.10
    int32_t vel = *(vel_ptr);
    int32_t pos = *(pos_ptr);

    
    float deltaTime = fabs(1/(float)vel);
    float omega = 2.0f / ((float)settings.damper.smoothTime / 1000);

    float x = omega * deltaTime;
    float exp = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);
    float change = (float)pos - (float)target; // floats so the rest result is not unsign
    int32_t originalTo = target;
     
    target = pos - change;
    int32_t prev_vel = vel;

    // Clamp maximum speed
    float maxChange = settings.motion.vel.max * settings.damper.smoothTime / 1000;
    change = fmax(-maxChange, fmin(change, maxChange));
    target = pos - change;
    float temp = (vel + omega * change) * deltaTime;
    vel = (int32_t)((vel - omega * temp) * exp);

    // establish minimum velocity to avoid too slow creep in
    if(vel < settings.motion.vel.min  && vel > 0) {
        vel = settings.motion.vel.min;
    } else if (vel > -settings.motion.vel.min && vel < 0) {
        vel = -settings.motion.vel.min;
    }

    if(vel == 0) {
        if (prev_vel > 0) {
            vel = -settings.motion.vel.min;
        } else {
            vel = settings.motion.vel.min;
        }
    }

    // increment system position
    if (vel > 0) {
        pos += 1;
        set_motor_direction(settings.motion.dir);
    } else {
        pos -= 1;
        set_motor_direction(!settings.motion.dir);
    }

    if(vel * prev_vel < 0) {
        invert_motor_direction(); 
    }

    *(vel_ptr) = vel;
    *(pos_ptr) = pos;
}

void update_velocity_exact(uint32_t target, int32_t *pos_ptr, int32_t *vel_ptr)
{
    // Based on Unity SmoothDamp() implementation of the critical 
    // Based on Game Programming Gems 4 Chapter 1.10
    int32_t vel = *(vel_ptr);
    int32_t pos = *(pos_ptr);

    
    float deltaTime = fabs(1/(float)sys.status.vel);
    float omega = 2.0f / ((float)settings.damper.smoothTime / 1000);

    float x = omega * deltaTime;
    float exp = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);
    float change = (float)pos - (float)target; // floats so the rest result is not unsign


    // Clamp maximum speed
    float maxChange = pulses_to_steps_u(settings.motion.vel.max) * settings.damper.smoothTime / 1000;
    change = fmax(-maxChange, fmin(change, maxChange));
    target = pos - change;

    float temp = (vel + omega * change) * deltaTime;
    vel = (int32_t)((vel - omega * temp) * exp);
    pos = (int32_t)(target + (change + temp) * exp);
    

    // establish minimum velocity to avoid too slow creep in
    if(vel < settings.motion.vel.min  && vel > 0) {
        vel = settings.motion.vel.min;
    } else if (vel > -settings.motion.vel.min && vel < 0) {
        vel = -settings.motion.vel.min;
    }

    //compensate for truncation
    if (vel < 0) {
        pos += 1;
    }

    *(vel_ptr) = vel;
    *(pos_ptr) = pos;
}