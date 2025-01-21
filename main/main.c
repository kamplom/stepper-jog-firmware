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
#include "settings.h"
#include "nvs_f.h"
#include "u_convert.h"

static const char *TAG = "main";

void app_main(void)
{
    start_up_sequence();
    // variables for main loop
    uint32_t symbol_duration;
    symbol_duration = 0;
    uint32_t iterations = 0;
    rmt_symbol_word_t symbol;
    symbol.level0 = 0;
    symbol.level1 = 1;
    
    // main loop. If target is not pos exectues whatever update_velocity tells it to.

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
            update_real_pos();
            sys.status.pos = sys.real.pos;
            ESP_LOGI(TAG, "state: jogging");
            // exit the inner while loop when 
            // we might need to jog
            if (abs(sys.real.pos - (int32_t)sys.target.pos) > 15) {
                motor_enabler(true);
                if(sys.target.pos > sys.status.pos) {
                    sys.status.vel = settings.motion.vel.min;
                    sys.hard_limit_pin = settings.gpio.limit_max;
                } else {
                    sys.status.vel = -settings.motion.vel.min;
                    sys.hard_limit_pin = settings.gpio.limit_min;
                }
                iterations = 0;

                // keep doing steps until we reach the desired position
                ESP_LOGI(TAG, "Jogging to %"PRIu32" from %"PRIu32, sys.target.pos, sys.status.pos);
                jog_aux.aux.pos = pulses_to_steps(sys.status.pos);
                jog_aux.aux.vel = pulses_to_steps(sys.status.vel);
                jog_aux.status.pos = jog_aux.aux.pos;
                jog_aux.status.vel = jog_aux.aux.vel;

                //loop until we reach the target position. transmit a step each iteration
                while (abs(sys.real.pos - (int32_t)sys.target.pos) > 1 && gpio_get_level(sys.hard_limit_pin)) {
                    //first symbol is sent without setting the motor direction
                    symbol_duration = (settings.rmt.motor_resolution / (uint32_t)abs(jog_aux.status.vel) / 2);
                    symbol.duration0 = symbol_duration;
                    symbol.duration1 = symbol_duration;
                    rmt_transmit(motor_chan, stepper_encoder, &symbol, sizeof(rmt_symbol_word_t), &tx_config);
                    //calc next symbol, it will increase position
                    update_velocity_exact(pulses_to_steps_u(sys.target.pos), &jog_aux.aux.pos, &jog_aux.aux.vel);
                    update_velocity((uint32_t)jog_aux.aux.pos, &jog_aux.status.pos, &jog_aux.status.vel);
                    
                    iterations += 1;
                    update_real_pos();
                    //jog_aux.status.pos = pulses_to_steps(sys.real.pos);
                    sys.status.pos = sys.real.pos;
                    //ESP_LOGI(TAG,"%"PRIi32" > %"PRIi32, sys.target.pos, sys.real.pos / 4);
                }

                sys.status.vel = 0;
                ESP_LOGI(TAG, "Iterations %lu",iterations);
                ESP_LOGI(TAG, "sys.target.pos: %"PRIu32, sys.target.pos);
                set_state(STATE_IDLE);
                motor_enabler(false);
            } else {
                set_state(STATE_IDLE);
                motor_enabler(false);
                //reset target, just in case
                update_real_pos();
                sys.target.pos = sys.real.pos;
            }
        } else if (sys.state & STATE_WHEEL) {
            ESP_LOGI(TAG, "pos: %"PRIu32, sys.status.pos);
            /* if (!((sys.status.pos == 0 && sys.wheel.vel < 0) || (sys.status.pos == (uint32_t)(500*STEPS_PER_MM) && sys.wheel.vel > 0))) {
                motor_enabler(true);
            }
            float smooth_vel = sys.status.vel;
            float smooth_acc = sys.status.acc;
            uint32_t smooth_pos = sys.status.pos;
            int32_t update_vel = sys.status.vel;
            uint32_t update_pos = sys.status.pos;
            float update_acc = sys.status.acc;
            //set dir and st v to zero
            if(sys.wheel.vel > 0) {
                sys.status.vel = 0;
                gpio_set_level(STEP_MOTOR_GPIO_DIR, STEP_MOTOR_SPIN_DIR_CLOCKWISE);
                vTaskDelay(pdMS_TO_TICKS(20));
            } else {
                sys.status.vel = 0;
                gpio_set_level(STEP_MOTOR_GPIO_DIR, STEP_MOTOR_SPIN_DIR_COUNTERCLOCKWISE);
                vTaskDelay(pdMS_TO_TICKS(20));
            }
            
            while((sys.wheel.vel != 0 || sys.status.vel != 0)) {
                sys.prev_status.vel = sys.status.vel;
                if (sys.status.vel == 0) {
                    if (sys.wheel.vel > 0) {
                        update_vel = STEPS_PER_MM*INITIAL_VELOCITY;
                    } else {
                        update_vel = -STEPS_PER_MM*INITIAL_VELOCITY;
                    }
                } else {
                    update_vel = sys.status.vel;
                }
                smooth_vel = sys.status.vel;
                smooth_acc = sys.status.acc;
                smooth_pos = sys.status.pos;
                update_pos = sys.status.pos;
                update_acc = sys.status.acc;
                //ESP_LOGI(TAG, "sys pos %"PRIu32, sys.status.pos);
                //ESP_LOGI(TAG, "wheel speed: %f", sys.wheel.vel);
                //ESP_LOGI(TAG, "sys vel: %f", sys.status.vel);
                smooth_damp(sys.wheel.vel, &smooth_vel, &smooth_acc);
                //ESP_LOGI(TAG, "smooth vel: %f", smooth_vel);
                if (smooth_vel < 0) {
                    update_velocity((uint32_t)0, &update_pos, &update_vel);
                    if (update_vel > 0) {
                        update_vel = -MIN_FEED_RATE;
                        update_acc = 0;
                    }
                    //check if the sign is actually correct, update-vel can overshoot
                    if (smooth_vel > update_vel) {
                        sys.status.vel = smooth_vel;
                        sys.status.acc = smooth_acc;
                    } else {
                        sys.status.vel = update_vel;
                        sys.status.acc = update_acc;
                    }
                    //ESP_LOGI(TAG, "smooth vel neg. update-vel: %f", update_vel);
                    //ESP_LOGI(TAG, "smooth vel neg. smooth-vel: %f", smooth_vel);
                } else if (smooth_vel > 0) {
                    update_velocity((uint32_t)(500*STEPS_PER_MM), &update_pos, &update_vel);
                    if (update_vel < 0) {
                        update_vel = MIN_FEED_RATE;
                        update_acc = 0;
                    }
                    if (smooth_vel < update_vel) {
                        sys.status.vel = smooth_vel;
                        sys.status.acc = smooth_acc;
                    } else {
                        sys.status.vel = update_vel;
                        sys.status.acc = update_acc;
                    }
                    //ESP_LOGI(TAG, "smooth vel pos. update-vel: %f", update_vel);
                    //ESP_LOGI(TAG, "smooth vel pos. smooth-vel: %f", smooth_vel);
                    //ESP_LOGI(TAG, "whell vel pos. update-vel: %f", update_vel);
                } else {
                    sys.status.vel = 0;
                    sys.status.acc = 0;
                }
                //sys.status.vel = smooth_vel;
                //sys.status.acc = smooth_acc;
                //ESP_LOGI(TAG, "update vel: %f", update_vel);
                //ESP_LOGI(TAG, "vel: %f", sys.status.vel);
                //sys.status.vel = fmin(update_vel, smooth_vel);
                symbol_duration = fabs(STEP_MOTOR_RESOLUTION_HZ / (float)(sys.status.vel) / 2);
                symbol.duration0 = symbol_duration;
                symbol.duration1 = symbol_duration;
                //ESP_LOGI(TAG, "vel: %f", sys.status.vel);

                if (sys.status.pos == 0 && sys.status.vel < 0) {
                    sys.status.vel = 0;
                    sys.status.acc = 0;
                } else if (sys.status.pos == (uint32_t)(500*STEPS_PER_MM) && sys.status.vel > 0) {
                    ESP_LOGI(TAG, "heree we go");
                    sys.status.vel = 0;
                    sys.status.acc = 0;
                }
                // if(sys.status.vel < MIN_FEED_RATE  && sys.status.vel > 0) {
                //     sys.status.vel = MIN_FEED_RATE;
                // } else if (sys.status.vel > -MIN_FEED_RATE && sys.status.vel < 0) {
                //     sys.status.vel = -MIN_FEED_RATE;
                // }
                if (sys.status.vel > 0) {
                    sys.status.pos += 1;
                } else if (sys.status.vel < 0){
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
                if (sys.status.vel != 0) {
                    rmt_transmit(motor_chan, stepper_encoder, &symbol, sizeof(rmt_symbol_word_t), &tx_config);
                }
            } */
            set_state(STATE_IDLE);
            motor_enabler(false);
        } else {
            vTaskDelay(pdMS_TO_TICKS(10));
            //motor_enabler(false);
        }
    }
}