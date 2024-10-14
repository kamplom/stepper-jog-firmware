/*
 * Config file for parameters.
 *
 * Eventually will contain defaults and the config will be changable at run time
 */
#include "driver/timer.h"

#define STEPS_PER_MM                53.3333333
#define MAX_FEED_RATE               200 //mm/s
#define MAX_ACCEL                   800 //mm/s^2
#define INITIAL_VELOCITY            10  //mm/s
#define MIN_FEED_RATE               50  //mm/s
//Rmt config
#define RMT_MEM_BLOCK_SYMBOLS      64
#define RMT_TRANS_QUEUE_DEPTH      1
#define EMERGENCY_STOP_COMMAND     "emergency"
#define JOG_CANCEL_COMMAND         "jog-cancel"
#define HOMING_COMMAND             "h"
#define EOL_COMMAND                "endofline"
#define EOLR_COMMAND               "return/r"

#define HOMING_FAST_SPEED           100
#define HOMING_SLOW_SPEED           25
#define HOMING_RETRACTION_DISTANCE  2 //mm

#define WHEEL_ENCODER_A             12
#define WHEEL_ENCODER_B             11
#define WHEEL_TIMER_INTERVAL        25 //miliseconds

#define STATE_ALERT                (1 << 0)
#define STATE_IDLE                 (1 << 1)
#define STATE_JOGGING              (1 << 2)
#define STATE_HOMING               (1 << 3)
#define STATE_WHEEL                (1 << 4)

#define ENABLE_DELAY             200 //ms
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
