#include "system.h"
#define STEPSMM_SHIFT 14
#define STEPSMM_SHIFT_MASK ((1 << STEPSMM_SHIFT) - 1)

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
    Setting_ChangeDirDelay = 9,
    // Homing seciton
    Setting_HomingFastVelocity = 10,
    Setting_HomingSlowVelocity = 11,
    Setting_HomingRetractionDistance = 12,
    //GPIO
    Setting_GpioMotorEn = 20,
    Setting_GpioMotorDir = 21,
    Setting_GpioMotorStep = 22,
    Setting_GpioMinLimit = 23,
    Setting_GpioMaxLimit = 24,
    Setting_GpioWheelA = 25,
    Setting_GpioWheelB = 26,
    //RMT
    Setting_RmtStepMotorResolution = 40,
    Setting_RmtQueueDepth = 41,
    Setting_RmtMemBlockSymbols = 42,
    //wheel
    Setting_WheelTimerActivate = 50,
    Setting_WheelTimerInterval = 51,
    //comands
    Setting_JogCancelCmd = 60,
    Setting_HomingCmd = 61,
} setting_id_t;

typedef enum  {
    Format_Bool = 0,
    Format_Int,
    Format_Float,
    Format_String,
    Format_Char,
} setting_datatype_t;

typedef struct  {
    uint32_t max;
    uint32_t min;
} minmax_t;

typedef struct  {
    minmax_t pos;
    minmax_t vel;
    minmax_t acc;
    uint32_t enable_delay;
    float steps_mm;
    uint32_t fixedp_steps_mm;
    bool dir;
    uint32_t dir_delay;
} motion_settings_t;

typedef struct {
    uint32_t fast_vel;
    uint32_t slow_vel;
    uint32_t retraction;
    bool direction;
} homing_settings_t;

typedef struct {
    uint32_t motor_en;
    uint32_t motor_dir;
    uint32_t motor_step;
    uint32_t limit_max;
    uint32_t limit_min;
    uint32_t wheel_A;
    uint32_t wheel_B;
} gpio_settings_t;

typedef struct {
    uint32_t motor_resolution;
    uint32_t queue_depth;
    uint32_t mem_block_sym;
} rmt_settings_t;

typedef struct {
    bool timer_activate;
    uint32_t timer_interval;
} wheel_settings_t;

typedef struct {
    char jog_cancel;
    char homing;
} cmd_settings_t;

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
    cmd_settings_t cmd;
} settings_t;

typedef struct setting_detail {
    setting_id_t id;
    const char *key;
    const char *unit;
    setting_datatype_t datatype;
    setting_datatype_t uinput_datatype;
    bool steps_multiply;
    uint32_t min_value;
    uint32_t max_value;
    void *value;
} setting_detail_t;

extern settings_t settings;
extern const setting_detail_t setting_detail[];
extern uint32_t N_settings;

void report_setting_short(uint32_t id);
void report_all_short();
bool find_setting(uint32_t id, uint32_t *index);
bool set_setting(uint32_t id, char *str_value);
void settings_init(void);

uint32_t float_to_fixed(float num);
float fixed_to_float(uint32_t num);