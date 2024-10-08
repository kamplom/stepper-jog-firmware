/*
 * Config file for parameters.
 *
 * Eventually will contain defaults and the config will be changable at run time
 */


#define STEPS_PER_MM                53.3333333
#define MAX_FEED_RATE               3000 //mm/s
#define MAX_ACCEL                   800 //mm/s^2
//Rmt config
#define RMT_MEM_BLOCK_SYMBOLS      64
#define RMT_TRANS_QUEUE_DEPTH      1


#define STEP_MOTOR_GPIO_EN       6
#define STEP_MOTOR_GPIO_DIR      5
#define STEP_MOTOR_GPIO_STEP     4
#define STEP_MOTOR_ENABLE_LEVEL  0
#define STEP_MOTOR_SPIN_DIR_CLOCKWISE 0
#define STEP_MOTOR_SPIN_DIR_COUNTERCLOCKWISE !STEP_MOTOR_SPIN_DIR_CLOCKWISE

#define STEP_MOTOR_RESOLUTION_HZ 1000000 // 1MHz resolution. determines how long are the high and low levels of step pulse signal
// i need a function to calculate it in terms of belt pitch pulley steps etc.

#define UART_RX_BUF_SIZE    1024
#define UART_TX_BUF_SIZE    1024
#define UART_SEL_NUM        UART_NUM_0
