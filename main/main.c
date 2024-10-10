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

    //end of temporal
    // init rmt channel and encoder
    create_rmt_channel();
    create_rmt_encoder();

    init_uart();
    // init sys variables
    sys.target.pos = 0;
    sys.status.pos = 0;

    // variables for main loop
    uint32_t symbol_duration;
    symbol_duration = 0;
    int iterations = 0;
    rmt_symbol_word_t symbol;

    // main loop. If target is not pos exectues whatever update_velocity tells it to.
    while(1) {
        if (sys.target.pos == sys.status.pos) {
            gpio_set_level(STEP_MOTOR_GPIO_EN, !STEP_MOTOR_ENABLE_LEVEL);
            vTaskDelay(pdMS_TO_TICKS(100));
        } else {
            // Target neq Pos
            // Enable motor
            gpio_set_level(STEP_MOTOR_GPIO_EN, STEP_MOTOR_ENABLE_LEVEL);
            // Set initial velocity and acceleration
            if(sys.target.pos > sys.status.pos) {
                sys.status.vel = STEPS_PER_MM*INITIAL_VELOCITY;
            } else {
                sys.status.vel = -STEPS_PER_MM*INITIAL_VELOCITY;
            }
            sys.status.acc = 0;
            iterations = 0;
            // keep doing steps until we reach the desired position
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
            ESP_LOGI(TAG, "Iterations %d",iterations);
            ESP_LOGI(TAG, "sys.target.pos: %"PRIu32, sys.target.pos);
        }
    }
}