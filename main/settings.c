#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stdio.h>
#include "esp_log.h"
// include modules
#include "settings.h"
#include "system.h"
#include "nvs_f.h"

static const char *TAG = "Settings";

settings_t settings;

const setting_detail_t setting_detail[] = {
    // id                          | key             | unit      | datatype    | input type  | multiply | value pointer
    {Setting_Stepsmm,              "Steps_mm",       "N. Steps", Format_Int,   Format_Float, false,     &settings.motion.fixedp_steps_mm},
    {Setting_MinPos,               "MinPos",         "mm",       Format_Int,   Format_Float, true,      &settings.motion.pos.min},
    {Setting_MaxPos,               "MaxPos",         "mm",       Format_Int,   Format_Float, true,      &settings.motion.pos.max},
    {Setting_MinVel,               "MinVel",         "mm/s",     Format_Int,   Format_Float, true,      &settings.motion.vel.min},
    {Setting_MaxVel,               "MaxVel",         "mm/s",     Format_Int,   Format_Float, true,      &settings.motion.vel.max},
    {Setting_MinAcc,               "MinAcc",         "mm/s2",    Format_Int,   Format_Float, true,      &settings.motion.acc.min},
    {Setting_MaxAcc,               "MaxAcc",         "mm/s2",    Format_Int,   Format_Float, true,      &settings.motion.acc.max},
    {Setting_EnableDelay,          "DelayEN",        "ms",       Format_Int,   Format_Int,   false,     &settings.motion.enable_delay},
    {Setting_InvertDirection,      "DirInv",         "bool",     Format_Bool,  Format_Bool,  false,     &settings.motion.dir},
    {Setting_HomingFastVelocity,   "HomingFast",     "mm/s",     Format_Int,   Format_Float, true,      &settings.homing.fast_vel},
    {Setting_HomingSlowVelocity,   "HomingSlow",     "mm/s",     Format_Int,   Format_Float, true,      &settings.homing.slow_vel},
    {Setting_HomingRetractionDistance,"HRetraction", "mm",       Format_Int,   Format_Float, true,      &settings.homing.retraction},
    {Setting_HomingInvertDirection,"DirInv",         "bool",     Format_Bool,  Format_Bool,  false,     &settings.homing.direction},
    {Setting_GpioMotorEn,          "GpioMotorEn",    "pin",      Format_Int,   Format_Int,   false,     &settings.gpio.motor_en},
    {Setting_GpioMotorDir,         "GpioMotorDir",   "pin",      Format_Int,   Format_Int,   false,     &settings.gpio.motor_dir},
    {Setting_GpioMotorStep,        "GpioMotorStep",  "pin",      Format_Int,   Format_Int,   false,     &settings.gpio.motor_step},
    {Setting_GpioMinLimit,         "GpioMinLim",     "pin",      Format_Int,   Format_Int,   false,     &settings.gpio.limit_min},
    {Setting_GpioMaxLimit,         "GpioMaxLim",     "pin",      Format_Int,   Format_Int,   false,     &settings.gpio.limit_max},
    {Setting_GpioWheelA,           "GpioWheelA",     "pin",      Format_Int,   Format_Int,   false,     &settings.gpio.wheel_A},
    {Setting_GpioWheelB,           "GpioWheelB",     "pin",      Format_Int,   Format_Int,   false,     &settings.gpio.wheel_B},
    {Setting_RmtStepMotorResolution,"RmtResolution", "hz",       Format_Int,   Format_Int,   false,     &settings.rmt.motor_resolution},
    {Setting_RmtQueueDepth,        "RmtQueueDepth",  "ul",       Format_Int,   Format_Int,   false,     &settings.rmt.queue_depth},
    {Setting_WheelTimerActivate,   "WheelTimerAct",  "bool",     Format_Bool,  Format_Bool,  false,     &settings.wheel.timer_activate},
    {Setting_WheelTimerInterval,   "WheelTimerInt",  "ms",       Format_Int,   Format_Int,   false,     &settings.wheel.timer_interval},
    {Setting_RmtMemBlockSymbols,   "RmtMemBlockSym", "ul",       Format_Int,   Format_Int,   false,     &settings.rmt.mem_block_sym},
    {Setting_ChangeDirDelay,       "DirDelay",       "ms",       Format_Int,   Format_Int,   false,     &settings.motion.dir_delay},
    {Setting_SmoothTime,           "SmoothTime",     "ms",       Format_Int,   Format_Int,   false,     &settings.damper.smoothTime},
    {Setting_StepsRev,             "StepsRev",       "steps",    Format_Int,   Format_Int,   false,     &settings.units.steps_rev},
    {Setting_PulsesRev,            "PulsesRev",      "pulses",   Format_Int,   Format_Int,   false,     &settings.units.pulses_rev},
    {Setting_mmRev,                "mmRev",          "mm",       Format_Int,   Format_Int,   false,     &settings.units.mm_rev}
};

uint32_t N_settings = sizeof(setting_detail)/sizeof(setting_detail[0]);

bool find_setting(uint32_t id, uint32_t *index) {
    uint32_t i;
    for (i = 0; i < N_settings; i++) {
        if (setting_detail[i].id == id) {
            *index = i;
            return true;
        }
    }
    return false;
}

void report_setting_short(uint32_t id) {
    uint32_t index;
    if(!find_setting(id, &index)) {
        printf("Setting not found\n");
        return;
    }
    switch (setting_detail[index].datatype) {
        case Format_Float:
            printf("%s: %f\n", setting_detail[index].key, *(float *)setting_detail[index].value);
            break;
        case Format_Int:
            printf("%s: %"PRIu32"\n", setting_detail[index].key, *(uint32_t *)setting_detail[index].value);
            break;
        case Format_Bool:
            printf("%s: %d\n", setting_detail[index].key, *(bool *)setting_detail[index].value);
            break;
        default:
            printf("bad\n");
            break;
    }
}

void report_setting_long(uint32_t id) {
    uint32_t index;
    find_setting(id, &index);
}

void report_all_short() {
    for (uint32_t i = 0; i < N_settings; i++) {
        report_setting_short(i);
    }
}

uint32_t float_to_fixed(float num) {
    return (uint32_t)(num * (1 << STEPSMM_SHIFT));
}

float fixed_to_float(uint32_t num) {
    //uint32_t numandmask = num & STEPSMM_SHIFT_MASK;
    //ESP_LOGD(TAG, "num and mask: %"PRIu32, numandmask);
    //float decimal = (float)(numandmask / (float)(1 << STEPSMM_SHIFT));
    //ESP_LOGD(TAG, "decimal: %f", decimal);
    return ((float)(num >> STEPSMM_SHIFT) + (float)((num & STEPSMM_SHIFT_MASK) / (float)(1 << STEPSMM_SHIFT)));
}

void recompute_stepsmm(float old_stepsmm) {
    for(uint32_t i = 0; i < N_settings; i++) {
        if(setting_detail[i].steps_multiply) {
            *(uint32_t*)setting_detail[i].value = (uint32_t)(*(uint32_t *)setting_detail[i].value * settings.motion.steps_mm / old_stepsmm); 
        }
    }
}

bool set_setting(uint32_t id, char *str_value) {
    uint32_t index;
    if(!find_setting(id, &index)) {
        printf("setting not found\n");
        return false;
    }
    uint32_t u32_value = 0;
    ESP_LOGD(TAG, "Index: %"PRIu32, index);
    ESP_LOGD(TAG, "Id: %d", setting_detail[index].id);
    ESP_LOGD(TAG, "Str_value %s",str_value);
    // only the steps_mm setting has a float datatype inside the settings
    // handle it here to simplify code downstream
    if(setting_detail[index].id == Setting_Stepsmm) {
        ESP_LOGD(TAG,"setting is steps per mm");
        float float_value;
        if(!str_to_float(str_value, &float_value)) {
                printf("Value not a valid float\n");
                return false;
        }
        ESP_LOGD(TAG, "float: %f", float_value);
        
        ESP_LOGD(TAG, "float: %f", float_value);
        float old_steps_mm = settings.motion.steps_mm;
        settings.motion.steps_mm = float_value;
        settings.motion.fixedp_steps_mm = float_to_fixed(float_value);
        ESP_LOGD(TAG, "fixed: %"PRIu32, settings.motion.fixedp_steps_mm);
        ESP_LOGD(TAG, "float again: %f", fixed_to_float(settings.motion.fixedp_steps_mm));
        // write to nvs
        ESP_LOGD(TAG, "steps per mm correctly");
        //trigger recompute of all the quantities set on steps
        recompute_stepsmm(old_steps_mm);
        return true;
    }
    switch (setting_detail[index].uinput_datatype) {
        case Format_Int:
            if(!str_to_u32(str_value, &u32_value)){
                printf("Value: \"%s\" not a valid int\n", str_value);
                return false;
            }
            if(setting_detail[index].steps_multiply) {
                u32_value = (uint32_t)(u32_value * settings.motion.steps_mm);
            }
            break;
        case Format_Float:
            float float_value;
            if(!str_to_float(str_value, &float_value)) {
                printf("Value not a valid float");
                return false;
            }
            if(setting_detail[index].steps_multiply) {
                u32_value = (uint32_t)(float_value * settings.motion.steps_mm);
            }
            break;

        case Format_Bool:
            if(!str_to_u32(str_value, &u32_value)) {
                printf("Value: \"%s\" not a valid int\n", str_value);
                return false;
            }
            if(!(u32_value == 1 || u32_value == 0)) {
                printf("value not a bool\n");
                return false;
            }
            break;
        default:
            break;
    }
    // check max and min
    
    //write to ram
    switch (setting_detail[index].datatype) {
        case Format_Int:
            *(uint32_t*)setting_detail[index].value = u32_value;
            break;
        case Format_Float:
            printf("this will never happen");
            break;
        case Format_Bool:
            *(bool*)setting_detail[index].value = u32_value;
        default:
            break;
    }
    //write to nvs
    nvs_write_setting(setting_detail[index].id);
    return true; 
}

void settings_init(void) {
    // set settings to defaults
    settings.motion.steps_mm = STEPS_PER_MM;
    nvs_read_setting(Setting_Stepsmm);
    const settings_t settings_defaults = {
        .settings_modified = false,
        //motion
        .motion.steps_mm = STEPS_PER_MM,
        .motion.pos.min = MIN_POS_LIMIT * settings.motion.steps_mm,
        .motion.pos.max = MAX_POS_LIMIT * settings.motion.steps_mm,
        .motion.vel.min = MIN_FEED_RATE * settings.motion.steps_mm,
        .motion.vel.max = MAX_FEED_RATE * settings.motion.steps_mm,
        .motion.acc.max = MAX_ACCEL * settings.motion.steps_mm,
        .motion.acc.max = MIN_ACCEL * settings.motion.steps_mm,
        .motion.dir = INVERT_DIRECTION,
        .motion.enable_delay = ENABLE_DELAY,
        .motion.dir_delay = CHANGE_DIR_DELAY,
        // homing
        .homing.direction = HOMING_DIRECTION,
        .homing.fast_vel = HOMING_FAST_SPEED * settings.motion.steps_mm,
        .homing.slow_vel = HOMING_SLOW_SPEED * settings.motion.steps_mm,
        .homing.retraction = HOMING_RETRACTION_DISTANCE * settings.motion.steps_mm,
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
        .rmt.mem_block_sym = RMT_MEM_BLOCK_SYMBOLS,
        //wheel
        .wheel.timer_activate = WHEEL_TIMER_ACTIVATE,
        .wheel.timer_interval = WHEEL_TIMER_INTERVAL,
        //cmd
        .cmd.jog_cancel = JOG_CANCEL_COMMAND,
        .cmd.homing = HOMING_COMMAND,
        //damper
        .damper.smoothTime = SMOOTHTIME,
        //units
        .units.steps_rev = STEPS_PER_REV,
        .units.pulses_rev = PULSES_PER_REV,
        .units.mm_rev = MM_PER_REV
    };
    settings = settings_defaults;
    ESP_LOGD(TAG, "stepsmm: %f", settings.motion.steps_mm);
    settings.motion.fixedp_steps_mm = float_to_fixed(settings.motion.steps_mm);
    ESP_LOGD(TAG, "fixed stepsmm: %"PRIu32, settings.motion.fixedp_steps_mm);
    ESP_LOGD(TAG, "float again: %f", fixed_to_float(settings.motion.fixedp_steps_mm));
    // load settings from nvs if they exist
    nvs_init();
    nvs_read_all_settings();
    nvs_write_all_settings();
}