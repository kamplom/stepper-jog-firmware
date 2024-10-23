#include "system.h"


typedef enum {
    // Movement section
    Setting_Stepsmm = 0,
    Setting_MinPos = 1,
    Setting_MaxPos = 2,
    Setting_MinVel = 3,
    Setting_MaxVel = 4,
    Setting_MinAcc = 5,
    Setting_MaxAcc = 6,
    Setting_EnableDelay = 7,
    Setting_InvertDirection = 8,
    // Homing seciton
    Setting_HomingFastVelocity = 10,
    Setting_HomingSlowVelocity = 11,
    Setting_HomingRetractionDistance = 12,
    //GPIO
    Setting_GpioMotorEn = 20,
    Setting_GpioMotorDir = 21,
    Setting_GpioMotorStep = 22,
    Setting_StepMotorSpinDir = 23,
    Setting_GpioMinLimit = 24,
    Setting_GpioMaxLimit = 25,
    Setting_GpioWheelA = 26,
    Setting_GpioWheelB = 27,
    //RMT
    Setting_RmtStepMotorResolution = 40,
    Setting_RmtQueueDepth = 41,
    //wheel
    Setting_WheelTimerActivate = 50,
    Setting_WheelTimerInterval = 51,
} setting_id_t;

typedef enum  {
    Format_Bool = 0,
    Format_Int,
    Format_Float,
    Format_String,
} setting_datatype_t;

typedef struct  {
    uint32_t max;
    uint32_t min;
} minmax_t;

typedef struct  {
    minmax_t pos;
    minmax_t vel;
    minmax_t acc;
    uint16_t enable_delay;
    float steps_mm;
    bool dir;
} motion_settings_t;

typedef struct {
    uint32_t fast_vel;
    uint32_t slow_vel;
    uint32_t retraction;
    bool direction;
} homing_settings_t;

typedef struct {
    uint8_t motor_en;
    uint8_t motor_dir;
    uint8_t motor_step;
    uint8_t limit_max;
    uint8_t limit_min;
    uint8_t wheel_A;
    uint8_t wheel_B;
} gpio_settings_t;

typedef struct {
    uint32_t motor_resolution;
    uint32_t queue_depth;
} rmt_settings_t;

typedef struct {
    bool timer_activate;
    uint16_t timer_interval;
} wheel_settings_t;


// Struct that stores the settings. Used at runtime, loads values either from defaults or nvs
// Any spatial unit must be steps.
// Temporal units may be different
typedef struct {
    bool settings_modified;
    motion_settings_t motion;
    homing_settings_t homing;
    gpio_settings_t gpio;
    rmt_settings_t rmt;
    wheel_settings_t wheel;
} settings_t;

typedef struct setting_detail {
    setting_id_t id;
    const char *key;
    const char *unit;
    setting_datatype_t datatype;
    bool steps_multiply;
    const char *format;
    const char *min_value;
    const char *max_value;
    void *value;
} setting_detail_t;

settings_t settings;