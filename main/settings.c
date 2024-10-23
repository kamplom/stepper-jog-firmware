#include <stdlib.h>
#include <string.h>
// include modules
#include "settings.h"
#include "system.h"



const settings_t settings_defaults = {
    .settings_modified = false,
    //motion
    .motion.steps_mm = STEPS_PER_MM,
    .motion.pos.min = MIN_POS_LIMIT,
    .motion.pos.max = MAX_POS_LIMIT,
    .motion.vel.min = MIN_FEED_RATE,
    .motion.vel.max = MAX_FEED_RATE,
    .motion.acc.max = MAX_ACCEL,
    .motion.acc.max = MIN_ACCEL,
    .motion.dir = INVERT_DIRECTION,
    .motion.enable_delay = ENABLE_DELAY,
    // homing
    .homing.direction = HOMING_DIRECTION,
    .homing.fast_vel = HOMING_FAST_SPEED,
    .homing.slow_vel = HOMING_SLOW_SPEED,
    .homing.retraction = HOMING_RETRACTION_DISTANCE,
    //gpio
    .gpio.limit_max = LIMIT_SWITCH_MAX_GPIO,
    .gpio.limit_min = LIMIT_SWITCH_MIN_GPIO,
    .gpio.motor_dir = STEP_MOTOR_GPIO_DIR,
    .gpio.motor_en = STEP_MOTOR_GPIO_EN,
    .gpio.motor_step = STEP_MOTOR_GPIO_STEP,
    .gpio.wheel_A = WHEEL_ENCODER_A,
    .gpio.wheel_B = WHEEL_ENCODER_B,
    //rmt
    .rmt.motor_resolution = STEP_MOTOR_RESOLUTION_HZ,
    .rmt.queue_depth = RMT_TRANS_QUEUE_DEPTH,
    //wheel
    .wheel.timer_activate = WHEEL_TIMER_ACTIVATE,
    .wheel.timer_interval = WHEEL_TIMER_INTERVAL,
};
settings_t settings;

const setting_detail_t setting_detail[] = {
    // id, key, unit, datatype, steps_multiply, format, min_value, max_value, value ptr.
    {Setting_Stepsmm, "Steps_mm", "N. Steps", Format_Float, false,"#0.0000", "1", "500", &settings.motion.steps_mm},
    {Setting_MinPos, "MinPos", "mm", Format_Float, true, "#0.00000", "0", "100", &settings.motion.pos.min},
    {Setting_MaxPos, "MaxPos", "mm", Format_Float, true, "#0.00000", "150", "6000", &settings.motion.pos.max},
    {Setting_MinVel, "MinVel", "mm/s", Format_Float, true, "#0.00000", "0.5", "50", &settings.motion.vel.min},
    {Setting_MaxVel, "MaxVel", "mm/s", Format_Float, true, "#0.00000", "100", "1000", &settings.motion.vel.max},
    {Setting_MinAcc, "MinAcc", "mm/s2", Format_Float, true, "#0.0000", "1", "100000000", &settings.motion.acc.min},
    {Setting_MaxAcc, "MaxAcc", "mm/s2", Format_Float, true, "#0.0000", "1", "1000000000", &settings.motion.acc.max},
    {Setting_EnableDelay, "DelayEN", "ms", Format_Int, false, "#0.00000", "0", "1000", &settings.motion.enable_delay},
    {Setting_InvertDirection, "DirInv", "bool", Format_Bool, false, "#0.0000", NULL, NULL, &settings.motion.dir}
};