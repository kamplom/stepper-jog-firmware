// Include esp-idf lib
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <inttypes.h>
#include "driver/rmt_tx.h"
#include "driver/gpio.h"
#include <math.h>

// Inlcude modules
#include "jogging.h"
#include "config.h"
#include "system.h"
#include "serial.h"
#include "wheel.h"

static const char *TAG = "main";

void app_main(void)
{
    // BEGIN BOOT-TIME SET-UP 
    //temporal
    ESP_LOGI(TAG, "Initialize EN + DIR GPIO");
    gpio_config_t en_dir_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pin_bit_mask = 1ULL << STEP_MOTOR_GPIO_DIR | 1ULL << STEP_MOTOR_GPIO_EN,
    };
    ESP_ERROR_CHECK(gpio_config(&en_dir_gpio_config));


    gpio_config_t stops_gpio_config = {
        .mode = GPIO_MODE_INPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pin_bit_mask = 1ULL << LIMIT_SWITCH_MIN_GPIO | 1ULL << LIMIT_SWITCH_MAX_GPIO, 
    };
    ESP_ERROR_CHECK(gpio_config(&stops_gpio_config));
    motor_enabler(false);
    //end of temporal
    // init rmt channel and encoder
    create_rmt_channel();
    create_rmt_encoder();

    init_uart();

    // init sys variables
    sys.target.pos = 0;
    sys.status.pos = 0;
    sys.status.vel = 0;
    sys.state = STATE_ALERT;
    // variables for main loop
    uint32_t symbol_duration;
    symbol_duration = 0;
    int iterations = 0;
    rmt_symbol_word_t symbol;
    
    // main loop. If target is not pos exectues whatever update_velocity tells it to.
    pcnt_init();
    wheel_timer_init();
    
    while(1) {
        // if we are in alert we will keep falling into here
        if (sys.state & STATE_ALERT) {
            vTaskDelay(pdMS_TO_TICKS(10));
            //motor_enabler(false);
        } else if (sys.state & STATE_HOMING) {
            // run homing sequence
            motor_enabler(true);
            ESP_LOGI(TAG, "Starting homig sequence");
            homing();
            ESP_LOGI(TAG, "Ended homing sequence");
            motor_enabler(false);
        } else if (sys.state & STATE_JOGGING) {
            // exit the inner while loop when 
            // we might need to jog
            if (sys.target.pos != sys.status.pos) {
                // Target neq Pos
                // Enable motor
                motor_enabler(true);
                // Set initial velocity and acceleration
                if(sys.target.pos > sys.status.pos) {
                    sys.status.vel = STEPS_PER_MM*INITIAL_VELOCITY;
                } else {
                    sys.status.vel = -STEPS_PER_MM*INITIAL_VELOCITY;
                }
                sys.status.acc = 0;
                iterations = 0;
                // keep doing steps until we reach the desired position
                ESP_LOGI(TAG, "Jogging to %"PRIu32" from %"PRIu32, sys.target.pos, sys.status.pos);
                while (sys.target.pos != sys.status.pos) {
                    // send symobl
                    symbol_duration = fabs(STEP_MOTOR_RESOLUTION_HZ / sys.status.vel / 2);
                    symbol.duration0 = symbol_duration;
                    symbol.level0 = 0;
                    symbol.duration1 = symbol_duration;
                    symbol.level1 = 1;
                    rmt_transmit(motor_chan, stepper_encoder, &symbol, sizeof(rmt_symbol_word_t), &tx_config);
                    //calc next symbol, it will increase position
                    update_velocity();
                    //if (iterations == 15000) {
                    //    sys.target.pos = 0;
                    //}
                    iterations += 1;
                }
                sys.status.vel = 0;
                ESP_LOGI(TAG, "Iterations %d",iterations);
                ESP_LOGI(TAG, "sys.target.pos: %"PRIu32, sys.target.pos);
                set_state(STATE_IDLE);
                motor_enabler(false);
            } else {
                set_state(STATE_IDLE);
                motor_enabler(false);
            }
        } else if (sys.state & STATE_WHEEL) {
            motor_enabler(true);
            if(sys.wheel.vel > 0) {
                sys.status.vel = STEPS_PER_MM*INITIAL_VELOCITY;
                gpio_set_level(STEP_MOTOR_GPIO_DIR, STEP_MOTOR_SPIN_DIR_CLOCKWISE);
            } else {
                sys.status.vel = -STEPS_PER_MM*INITIAL_VELOCITY;
                gpio_set_level(STEP_MOTOR_GPIO_DIR, STEP_MOTOR_SPIN_DIR_COUNTERCLOCKWISE);
            }
            while(sys.wheel.vel != 0 || sys.status.vel != 0) {
                sys.prev_status.vel = sys.status.vel;
                smooth_damp();
                symbol_duration = fabs(STEP_MOTOR_RESOLUTION_HZ / sys.status.vel / 2);
                symbol.duration0 = symbol_duration;
                symbol.level0 = 0;
                symbol.duration1 = symbol_duration;
                symbol.level1 = 1;
                //ESP_LOGI(TAG, "vel: %f", sys.status.vel);
                if (sys.status.vel > 0) {
                    sys.status.pos += 1;
                } else {
                    sys.status.pos -= 1;
                }
                if(sys.status.vel * sys.prev_status.vel < 0) {
                    if(sys.status.vel > 0) {
                        gpio_set_level(STEP_MOTOR_GPIO_DIR, STEP_MOTOR_SPIN_DIR_CLOCKWISE);
                        vTaskDelay(pdMS_TO_TICKS(200));
                    } else {
                        gpio_set_level(STEP_MOTOR_GPIO_DIR, STEP_MOTOR_SPIN_DIR_COUNTERCLOCKWISE);
                        vTaskDelay(pdMS_TO_TICKS(200));
                    } 
                } 
                rmt_transmit(motor_chan, stepper_encoder, &symbol, sizeof(rmt_symbol_word_t), &tx_config);
            }
            set_state(STATE_IDLE);
            motor_enabler(false);
        } else {
            vTaskDelay(pdMS_TO_TICKS(10));
            //motor_enabler(false);
        }
    }
}