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
    int imod;
    int iterations;
    int invalid_steps;
    freq = 0;
    while(1) {
        if (sys.target.pos == sys.status.pos) {
            gpio_set_level(STEP_MOTOR_GPIO_EN, !STEP_MOTOR_ENABLE_LEVEL);
            vTaskDelay(pdMS_TO_TICKS(10));
        } else {
            gpio_set_level(STEP_MOTOR_GPIO_EN, STEP_MOTOR_ENABLE_LEVEL);
            invalid_steps = 0;
            aux_steps = sys.target.accel_steps + sys.target.constant_speed_steps;
            iterations = (int)(sys.target.total_steps / points_num)*points_num;
            ESP_LOGI(TAG, "iterations: %d", iterations);
            if (sys.target.total_steps%points_num  != 0) {
                iterations = iterations + points_num;
            }
            ESP_LOGI(TAG, "iterations: %d", iterations);
            for(int i = 0; i < iterations; i++){
                imod = i%points_num;
                if(i < sys.target.accel_steps){
                    freq = sys.target.vel * i /(sys.target.accel_steps-1);
                    freq = convert_to_smooth_freq(0, sys.target.vel, freq);
                    symbol_duration = STEP_MOTOR_RESOLUTION_HZ / freq / 2;
                    symbols[imod].duration0 = symbol_duration;
                    symbols[imod].duration1 = symbol_duration;
                    symbols[imod].level0 = 0;
                    symbols[imod].level1 = 1;
                } else if (i < aux_steps){
                    symbols[imod].duration0 = symbol_duration;
                    symbols[imod].duration1 = symbol_duration;
                    symbols[imod].level0 = 0;
                    symbols[imod].level1 = 1;
                } else if (i < sys.target.total_steps){
                    //deceleration phase
                    freq = sys.target.vel - sys.target.vel * (i-aux_steps) / (sys.target.accel_steps-1);
                    freq = convert_to_smooth_freq(0, sys.target.vel, freq);
                    symbol_duration = STEP_MOTOR_RESOLUTION_HZ / freq / 2;
                    symbols[imod].duration0 = symbol_duration;
                    symbols[imod].duration1 = symbol_duration;
                    symbols[imod].level0 = 0;
                    symbols[imod].level1 = 1;
                } else {
                    symbols[imod].duration0 = STEP_MOTOR_RESOLUTION_HZ/15000;
                    symbols[imod].duration1 = STEP_MOTOR_RESOLUTION_HZ/15000;
                    symbols[imod].level0 = 0;
                    symbols[imod].level1 = 0;
                    invalid_steps = invalid_steps + 1;
                }
                if ((i+1) % (points_num) == 0 && i != 0) {
                    ESP_LOGI(TAG, "freq: %lf",freq);
                    if(sys.target.dir) {
                        sys.status.pos = sys.status.pos + points_num - invalid_steps;    
                    } else {
                        sys.status.pos = sys.status.pos - points_num + invalid_steps;
                    }
                    
                    // ESP_LOGI(TAG, "transmit at i: %d", i);
                    // ESP_LOGI(TAG, "symbol0 half duration: %"PRIu16, symbols[0].duration0);
                    ESP_ERROR_CHECK(rmt_transmit(motor_chan, stepper_encoder, &symbols[0], sizeof(rmt_symbol_word_t)*points_num, &tx_config));
                }
            }
        }
        //get xfa. Compare with current status and decide what to do
        //generate frame
        //send frame
        // ESP_LOGI(TAG, "new frame sent");
        
    }

}