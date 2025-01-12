#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stdio.h>
#include <math.h>
#include "esp_log.h"
#include "settings.h"
#include "system.h"
#include "nvs_f.h"

#include "u_convert.h"

static const char *TAG = "Settings";

settings_t settings;

static const settings_t settings_defaults = {
    .settings_modified = false,
    //motion
    .motion.pos.min = MIN_POS_LIMIT,
    .motion.pos.max = MAX_POS_LIMIT,
    .motion.vel.min = MIN_FEED_RATE,
    .motion.vel.max = MAX_FEED_RATE,
    .motion.acc.max = MAX_ACCEL,
    .motion.acc.min = MIN_ACCEL,
    .motion.dir = INVERT_DIRECTION,
    .motion.enable_delay = ENABLE_DELAY,
    .motion.dir_delay = CHANGE_DIR_DELAY,
    // homing
    .homing.direction = HOMING_DIRECTION,
    .homing.fast_vel = HOMING_FAST_SPEED,
    .homing.slow_vel = HOMING_SLOW_SPEED,
    .homing.retraction = HOMING_RETRACTION_DISTANCE,
    .homing.offset = 667,
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
    .units.mm_rev = MM_PER_REV,
    .stream.serial_activate = 1,
    .stream.ws_activate = 0
};

const setting_detail_t setting_detail[] = {
    // id                          | key             |unit       | datatype    | input type |recompute| trigger | value pointer                    | default
    {Setting_MinPos,               "MinPos",         Unit_mm,    Format_Int,   Format_Int,   true,      false,    &settings.motion.pos.min,          &settings_defaults.motion.pos.min},
    {Setting_MaxPos,               "MaxPos",         Unit_mm,    Format_Int,   Format_Int,   true,      false,    &settings.motion.pos.max,          &settings_defaults.motion.pos.max},
    {Setting_MinVel,               "MinVel",         Unit_mm_s,  Format_Int,   Format_Int,   true,      false,    &settings.motion.vel.min,          &settings_defaults.motion.vel.min},
    {Setting_MaxVel,               "MaxVel",         Unit_mm_s,  Format_Int,   Format_Int,   true,      false,    &settings.motion.vel.max,          &settings_defaults.motion.vel.max},
    {Setting_MinAcc,               "MinAcc",         Unit_mm_ss, Format_Int,   Format_Int,   true,      false,    &settings.motion.acc.min,          &settings_defaults.motion.acc.min},
    {Setting_MaxAcc,               "MaxAcc",         Unit_mm_ss, Format_Int,   Format_Int,   true,      false,    &settings.motion.acc.max,          &settings_defaults.motion.acc.max},
    {Setting_EnableDelay,          "DelayEN",        Unit_ms,    Format_Int,   Format_Int,   false,     false,    &settings.motion.enable_delay,     &settings_defaults.motion.enable_delay},
    {Setting_InvertDirection,      "DirInv",         Unit_bool,  Format_Bool,  Format_Bool,  false,     false,    &settings.motion.dir,              &settings_defaults.motion.dir},
    {Setting_HomingFastVelocity,   "HomingFast",     Unit_mm_s,  Format_Int,   Format_Int,   true,      false,    &settings.homing.fast_vel,         &settings_defaults.homing.fast_vel},
    {Setting_HomingSlowVelocity,   "HomingSlow",     Unit_mm_s,  Format_Int,   Format_Int,   true,      false,    &settings.homing.slow_vel,         &settings_defaults.homing.slow_vel},
    {Setting_HomingRetractionDistance,"HRetraction", Unit_mm,    Format_Int,   Format_Int,   true,      false,    &settings.homing.retraction,       &settings_defaults.homing.retraction},
    {Setting_HomingInvertDirection,"DirInv",         Unit_bool,  Format_Bool,  Format_Bool,  false,     false,    &settings.homing.direction,        &settings_defaults.homing.direction},
    {Setting_GpioMotorEn,          "GpioMotorEn",    Unit_pin,   Format_Int,   Format_Int,   false,     false,    &settings.gpio.motor_en,           &settings_defaults.gpio.motor_en},
    {Setting_GpioMotorDir,         "GpioMotorDir",   Unit_pin,   Format_Int,   Format_Int,   false,     false,    &settings.gpio.motor_dir,          &settings_defaults.gpio.motor_dir},
    {Setting_GpioMotorStep,        "GpioMotorStep",  Unit_pin,   Format_Int,   Format_Int,   false,     false,    &settings.gpio.motor_step,         &settings_defaults.gpio.motor_step},
    {Setting_GpioMinLimit,         "GpioMinLim",     Unit_pin,   Format_Int,   Format_Int,   false,     false,    &settings.gpio.limit_min,          &settings_defaults.gpio.limit_min},
    {Setting_GpioMaxLimit,         "GpioMaxLim",     Unit_pin,   Format_Int,   Format_Int,   false,     false,    &settings.gpio.limit_max,          &settings_defaults.gpio.limit_max},
    {Setting_GpioWheelA,           "GpioWheelA",     Unit_pin,   Format_Int,   Format_Int,   false,     false,    &settings.gpio.wheel_A,            &settings_defaults.gpio.wheel_A},
    {Setting_GpioWheelB,           "GpioWheelB",     Unit_pin,   Format_Int,   Format_Int,   false,     false,    &settings.gpio.wheel_B,            &settings_defaults.gpio.wheel_B},
    {Setting_RmtStepMotorResolution,"RmtResolution", Unit_hz,    Format_Int,   Format_Int,   false,     false,    &settings.rmt.motor_resolution,    &settings_defaults.rmt.motor_resolution},
    {Setting_RmtQueueDepth,        "RmtQueueDepth",  Unit_ul,    Format_Int,   Format_Int,   false,     false,    &settings.rmt.queue_depth,         &settings_defaults.rmt.queue_depth},
    {Setting_WheelTimerActivate,   "WheelTimerAct",  Unit_bool,  Format_Bool,  Format_Bool,  false,     false,    &settings.wheel.timer_activate,    &settings_defaults.wheel.timer_activate},
    {Setting_WheelTimerInterval,   "WheelTimerInt",  Unit_ms,    Format_Int,   Format_Int,   false,     false,    &settings.wheel.timer_interval,    &settings_defaults.wheel.timer_interval},
    {Setting_RmtMemBlockSymbols,   "RmtMemBlockSym", Unit_ul,    Format_Int,   Format_Int,   false,     false,    &settings.rmt.mem_block_sym,       &settings_defaults.rmt.mem_block_sym},
    {Setting_ChangeDirDelay,       "DirDelay",       Unit_ms,    Format_Int,   Format_Int,   false,     false,    &settings.motion.dir_delay,        &settings_defaults.motion.dir_delay},
    {Setting_SmoothTime,           "SmoothTime",     Unit_ms,    Format_Int,   Format_Int,   false,     false,    &settings.damper.smoothTime,       &settings_defaults.damper.smoothTime},
    {Setting_StepsRev,             "StepsRev",       Unit_step,  Format_Int,   Format_Int,   false,     false,     &settings.units.steps_rev,         &settings_defaults.units.steps_rev},
    {Setting_PulsesRev,            "PulsesRev",      Unit_pulse, Format_Int,   Format_Int,   false,     true,     &settings.units.pulses_rev,        &settings_defaults.units.pulses_rev},
    {Setting_mmRev,                "mmRev",          Unit_mm,    Format_Int,   Format_Int,   false,     false,     &settings.units.mm_rev,            &settings_defaults.units.mm_rev}
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

uint32_t convert_to_report(uint32_t index) {
    if (setting_detail[index].recompute) {
        switch(setting_detail[index].unit) {
            case Unit_mm:
                return (uint32_t)round(pulses_to_mm(*(uint32_t*)setting_detail[index].value));
                break;            
            case Unit_mm_s:
                return (uint32_t)round(pulses_to_mm(*(uint32_t*)setting_detail[index].value));
                break;
            case Unit_mm_ss:
                return (uint32_t)round(pulses_to_mm(*(uint32_t*)setting_detail[index].value));
                break;
            default:
                return *(uint32_t*)setting_detail[index].value;
                break;
        }
    } else {
        return *(uint32_t*)setting_detail[index].value;
    }
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
            printf("%u=%"PRIu32"\n", setting_detail[index].id, convert_to_report(index));
            break;
        case Format_Bool:
            printf("%u=%d\n", setting_detail[index].id, *(bool *)setting_detail[index].value);
            break;
        default:
            printf("bad\n");
            break;
    }
}

void report_all_short(){
    for(uint32_t i = 0; i < N_settings; i++) {
        report_setting_short(setting_detail[i].id);
    }
}

char* unit_to_str(setting_unit_t unit) {
    switch(unit) {
        case Unit_mm: return "mm";
        case Unit_mm_s: return "mm/s";
        case Unit_mm_ss: return "mm/s2";
        case Unit_bool: return "bool";
        case Unit_pin: return "pin";
        case Unit_hz: return "hz";
        case Unit_ul: return "ul";
        case Unit_ms: return "ms";
        case Unit_step: return "step";
        case Unit_pulse: return "pulse";
        case Unit_step_mm: return "step/mm";
        default: return "unknown";
    }
}

char* format_to_str(setting_datatype_t format) {
    switch(format) {
        case Format_Int: return "Int";
        case Format_Float: return "Float";
        case Format_Bool: return "Bool";
        default: return "unknown";
    }
}
void report_setting_long_row(uint32_t id) {
    uint32_t index;
    if(!find_setting(id, &index)) {
        printf("Setting not found\n");
        return;
    }

    printf("║%-4u║%-15s║%-10s║%-10s║%-10s║%-8s║%-8s║%-10lu║%-10lu║\n",
        setting_detail[index].id,
        setting_detail[index].key,
        unit_to_str(setting_detail[index].unit),
        format_to_str(setting_detail[index].datatype),
        format_to_str(setting_detail[index].uinput_datatype),
        setting_detail[index].recompute ? "true" : "false", 
        setting_detail[index].trigger ? "true" : "false",
        *(uint32_t*)setting_detail[index].value,
        convert_to_report(index)
    );
}

void report_setting_header(void) {
    printf("\n╔════╦═══════════════╦══════════╦══════════╦══════════╦════════╦════════╦══════════╦══════════╗\n");
    printf(  "║ ID ║     KEY       ║ INPUT U. ║ DATATYPE ║  INPUT   ║ RECOMP ║TRIGGER ║   RAW    ║   CONV   ║\n");
    printf(  "╠════╬═══════════════╬══════════╬══════════╬══════════╬════════╬════════╬══════════╬══════════╣\n");
}

void report_setting_footer(void) {
    printf(  "╚════╩═══════════════╩══════════╩══════════╩══════════╩════════╩════════╩══════════╩══════════╝\n");
}

void report_setting_long(uint32_t id) {
    report_setting_header();
    report_setting_long_row(id);
    report_setting_footer();
}

void report_all_long(void) {

    report_setting_header();
    for(uint32_t i = 0; i < N_settings; i++) {
        report_setting_long_row(setting_detail[i].id);
    }
    report_setting_footer();    
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

void recompute_stepsmm(uint32_t old) {
    for(uint32_t i = 0; i < N_settings; i++) {
        if(setting_detail[i].recompute) {
            *(uint32_t*)setting_detail[i].value = (uint32_t)(*(uint32_t *)setting_detail[i].value * settings.units.pulses_rev / old); 
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
    uint32_t old_value = 0;
    // only the steps_mm setting has a float datatype inside the settings
    // handle it here to simplify code downstream
    if(setting_detail[index].trigger) {
        old_value = *(uint32_t*)setting_detail[index].value;
    }
    switch (setting_detail[index].uinput_datatype) {
        case Format_Int:
            if(!str_to_u32(str_value, &u32_value)){
                printf("Value: \"%s\" not a valid int\n", str_value);
                return false;
            }
            if(setting_detail[index].recompute) {
                u32_value = mm_to_pulses(u32_value);
            }
            break;
        case Format_Float:
            printf("impossible!\n");
            break;
        case Format_Bool:
            if(!str_to_u32(str_value, &u32_value)){
                printf("Value: \"%s\" not a valid int\n", str_value);
                return false;
            }
            if(setting_detail[index].recompute) {
                u32_value = mm_to_pulses(u32_value);
            }
            break;
        default:
            break;
    }
    //write to ram
    switch (setting_detail[index].datatype) {
        case Format_Int:
            *(uint32_t*)setting_detail[index].value = u32_value;
            break;
        case Format_Float:
            printf("this will never happen");
            break;
        case Format_Bool:
            *(bool*)setting_detail[index].value = (bool)u32_value;
        default:
            break;
    }

    if(setting_detail[index].trigger) {
        recompute_stepsmm(old_value);
    }

    //write to nvs
    nvs_write_setting(setting_detail[index].id);
    report_setting_long(setting_detail[index].id);
    return true; 
}

void settings_init(void) {
    nvs_init();
    // default settings that trigger recompute
    settings.units.steps_rev = settings_defaults.units.steps_rev;
    settings.units.pulses_rev = settings_defaults.units.pulses_rev;
    settings.units.mm_rev = settings_defaults.units.mm_rev;
    // not a great way, put the default values in the settings_details struct
    // read settings that trigger recompute
    for(uint32_t i = 0; i < N_settings; i++) {
        if(setting_detail[i].trigger) {
            nvs_read_setting(setting_detail[i].id);
        }
    }
    
    settings = settings_defaults;
    //recompute needed settings
    for (uint32_t i = 0; i < N_settings; i++) {
        if(setting_detail[i].recompute) {
            switch(setting_detail[i].datatype) {
                case Format_Int:
                    ESP_LOGI(TAG, "value: %"PRIu32, *(uint32_t*)setting_detail[i].value);
                    switch (setting_detail[i].unit) {
                        case Unit_mm:
                            *(uint32_t*)setting_detail[i].value = mm_to_pulses(*(uint32_t*)setting_detail[i].value);
                            break;
                        case Unit_mm_s:
                            *(uint32_t*)setting_detail[i].value = mm_to_pulses(*(uint32_t*)setting_detail[i].value);
                            break;
                        case Unit_mm_ss:
                            *(uint32_t*)setting_detail[i].value = mm_to_pulses(*(uint32_t*)setting_detail[i].value);
                            break;
                        case Unit_step_mm:
                            printf("impossible should be other\n");
                            break;
                        case Unit_pulse:
                            printf("impossible shoudl be other\n");
                            break;
                        default:
                            printf("impossible shoudl be other\n");
                            break;
                    }
                    break;
                default:
                    printf("impossible shoudl be int\n");
                    break;
            }
        }
    }
    // load settings from nvs if they exist
    nvs_read_all_settings();
    nvs_write_all_settings();
}