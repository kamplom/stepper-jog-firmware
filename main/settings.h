typedef enum {
    // Movement section
    Setting_Stepsmm = 0,
    Setting_MaxVelocity= 1,
    Setting_MaxAcceleration = 2,
    Setting_InitialVelocity = 3,
    Setting_MinVelocity = 4,
    Setting_EnableDelay = 5,
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
} setting_datatype_t;

typedef struct setting_detail {
    setting_id_t id;
    const char *name;
    const char *unit;
    setting_datatype_t datatype;
    const char *format;
    const char *min_value;
    const char *max_value;
    void *value;
    void *get_value;
} setting_detail_t;