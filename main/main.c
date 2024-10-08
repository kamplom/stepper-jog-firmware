#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "system.h"
#include "stepper_encoder.h"
#include "esp_log.h"
#include <inttypes.h>
#include "driver/rmt_tx.h"
#include "driver/gpio.h"
#include "serial.h"
#include <math.h>

#include "config.h"

static const char *TAG = "main";


rmt_symbol_word_t *symbols;

void app_main(void)
{
    
    int points_num = 100;
    rmt_symbol_word_t *symbols;
    symbols = (rmt_symbol_word_t*)rmt_alloc_encoder_mem(sizeof(rmt_symbol_word_t)*points_num);
    // uint32_t symbol_duration = 10 / 90000 / 2;
    // symbol_duration = STEP_MOTOR_RESOLUTION_HZ / 9000 / 2;

    // for (int i = 0; i < points_num; i++)
    // {
    //     symbols[i].duration0 = symbol_duration;
    //     symbols[i].duration1 = symbol_duration;
    //     symbols[i].level0 = 0;
    //     symbols[i].level1 = 1;
    // }

    //temporal
    ESP_LOGI(TAG, "Initialize EN + DIR GPIO");
    gpio_config_t en_dir_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pin_bit_mask = 1ULL << STEP_MOTOR_GPIO_DIR | 1ULL << STEP_MOTOR_GPIO_EN,
    };
    ESP_ERROR_CHECK(gpio_config(&en_dir_gpio_config));

    //end of temporal
    create_rmt_channel();
    create_rmt_encoder();

    init_uart();
    sys.target.pos = 0;
    sys.status.pos = 0;
    double freq;
    uint32_t symbol_duration;
    uint32_t aux_steps;
    symbol_duration = 0;
    int imod = 0;
    int iterations = 0;
    int invalid_steps;
    freq = 0;
    rmt_symbol_word_t symbol;
    while(1) {
        if (sys.target.pos == sys.status.pos) {
            gpio_set_level(STEP_MOTOR_GPIO_EN, !STEP_MOTOR_ENABLE_LEVEL);
            vTaskDelay(pdMS_TO_TICKS(100));
        } else {
            gpio_set_level(STEP_MOTOR_GPIO_EN, STEP_MOTOR_ENABLE_LEVEL);
            if(sys.target.pos > sys.status.pos) {
                sys.status.vel = STEPS_PER_MM*10;
            } else {
                sys.status.vel = -STEPS_PER_MM*10;
            }
            sys.status.acc = 0;
            
            while (sys.target.pos != sys.status.pos) {
                symbol_duration = fabs(STEP_MOTOR_RESOLUTION_HZ / sys.status.vel / 2);
                symbol.duration0 = symbol_duration;
                symbol.level0 = 0;
                symbol.duration1 = symbol_duration;
                symbol.level1 = 1;
                rmt_transmit(motor_chan, stepper_encoder, &symbol, sizeof(rmt_symbol_word_t), &tx_config);
                SmoothDamp();
                iterations += 1;
            }
            ESP_LOGI(TAG, "Iterations %d",iterations);
            ESP_LOGI(TAG, "sys.target.pos: %"PRIu32, sys.target.pos);
        }
        //get xfa. Compare with current status and decide what to do
        //generate frame
        //send frame
        // ESP_LOGI(TAG, "new frame sent");
        
    }

}