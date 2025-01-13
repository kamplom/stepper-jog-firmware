/*
 * Config file for parameters.
 *
 * Eventually will contain defaults and the config will be changable at run time
 */
#include "driver/timer.h"

#define STEPS_PER_MM                53.3333333

#define STEPS_PER_REV               2000
#define PULSES_PER_REV              4000
#define MM_PER_REV                  60

#define MAX_FEED_RATE               200 //mm/s
#define MAX_ACCEL                   800 //mm/s^2
#define MIN_ACCEL                   0
#define INITIAL_VELOCITY            2  //mm/s
#define MIN_FEED_RATE               5  //mm/s
#define MIN_POS_LIMIT               0
#define MAX_POS_LIMIT               1300
#define INVERT_DIRECTION            false
#define SMOOTHTIME                  400
#define JOG_CANCEL_DISTANCE         75 //mm

//Rmt config
#define RMT_MEM_BLOCK_SYMBOLS      64
#define RMT_TRANS_QUEUE_DEPTH      1
#define EMERGENCY_STOP_COMMAND     'e'
#define JOG_CANCEL_COMMAND         'j'
#define HOMING_COMMAND             'h'
#define EOL_COMMAND                "endofline"
#define EOLR_COMMAND               "return/r"

#define HOMING_FAST_SPEED           25
#define HOMING_SLOW_SPEED           5
#define HOMING_RETRACTION_DISTANCE  2 //mm
#define HOMING_DIRECTION            false
#define HOMING_OFFSET               10 //mm

#define WHEEL_ENCODER_A             12
#define WHEEL_ENCODER_B             11
#define WHEEL_TIMER_INTERVAL        400 //miliseconds
#define WHEEL_TIMER_ACTIVATE        true

#define STATE_ALERT                (1 << 0)
#define STATE_IDLE                 (1 << 1)
#define STATE_JOGGING              (1 << 2)
#define STATE_HOMING               (1 << 3)
#define STATE_WHEEL                (1 << 4)

#define ENABLE_DELAY             650 //ms
#define CHANGE_DIR_DELAY         250 //ms
#define STEP_MOTOR_GPIO_EN       6
#define STEP_MOTOR_GPIO_DIR      5
#define STEP_MOTOR_GPIO_STEP     4
#define STEP_MOTOR_ENABLE_LEVEL  0
#define STEP_MOTOR_SPIN_DIR_CLOCKWISE 0
#define STEP_MOTOR_SPIN_DIR_COUNTERCLOCKWISE !STEP_MOTOR_SPIN_DIR_CLOCKWISE

#define LIMIT_SWITCH_MIN_GPIO   10
#define LIMIT_SWITCH_MAX_GPIO   9 

#define STEP_MOTOR_RESOLUTION_HZ 1000000 // 1MHz resolution. determines how long are the high and low levels of step pulse signal
// i need a function to calculate it in terms of belt pitch pulley steps etc.

#define UART_RX_BUF_SIZE    1024
#define UART_TX_BUF_SIZE    1024
#define UART_SEL_NUM        UART_NUM_0

#define WS_STREAM_ACTIVATE      false
#define SERIAL_STREAM_ACTIVATE  true