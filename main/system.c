// Include esp-idf libraries
#include "driver/rmt_tx.h"
#include "esp_log.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <inttypes.h>
#include <math.h>
#include "driver/gpio.h"

// Include modules
#include "system.h"
#include "config.h"
static const char *TAG = "System";

rmt_transmit_config_t tx_config = {};
rmt_channel_handle_t motor_chan = NULL;
rmt_encoder_handle_t stepper_encoder = NULL;
QueueHandle_t uart_queue = NULL;
system_t sys = {};

void mm_to_steps(float *mm, uint32_t *steps) {
    *steps = (uint32_t)(*mm * STEPS_PER_MM);
}

float convert_to_smooth_freq(uint32_t freq1, uint32_t freq2, uint32_t freqx)
{
    float normalize_x = ((float)(freqx - freq1)) / (freq2 - freq1);
    // third-order "smoothstep" function: https://en.wikipedia.org/wiki/Smoothstep
    float smooth_x = normalize_x * normalize_x * (3 - 2 * normalize_x);
    // fifth-order "smoothstep" function: https://en.wikipedia.org/wiki/Smoothstep
    // float smooth_x = normalize_x * normalize_x * normalize_x * (normalize_x * (normalize_x * 6 - 15) + 10);
    // float smooth_x = normalize_x;
    return smooth_x * (freq2 - freq1) + freq1;
}

void create_rmt_channel(void)
{
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT, // select clock source
        .gpio_num = STEP_MOTOR_GPIO_STEP,
        .mem_block_symbols = RMT_MEM_BLOCK_SYMBOLS,
        .resolution_hz = STEP_MOTOR_RESOLUTION_HZ,
        .trans_queue_depth = RMT_TRANS_QUEUE_DEPTH, // set the number of transactions that can be pending in the background
    };
    ESP_LOGI(TAG, "Create RMT TX channel");
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &motor_chan));
    ESP_LOGI(TAG, "Enable RMT channel");
    ESP_ERROR_CHECK(rmt_enable(motor_chan));
    
}

void create_rmt_encoder(void) {
    tx_config.loop_count = 0;
    tx_config.flags.queue_nonblocking = false;
    rmt_copy_encoder_config_t copy_encoder_config = {};
    rmt_new_copy_encoder(&copy_encoder_config, &stepper_encoder);
}


//migrate atoi to strtol, then convert to steps and then convert to int
//Implement default values as half of the max when no value is provided
void parse_command(const char *command, uint32_t *xVal, uint32_t *fVal, uint32_t *aVal, bool *is_incremental) {
    char *copy = strdup(command);  // Make a copy of the command to avoid modifying the original
    char *token;
    char *endptr;
    // Initialize the is_incremental flag to false (default to absolute movement)
    *is_incremental = false;
    // Find the 'X' value
    token = strchr(copy, 'X');
    if (token != NULL) {
        // Check for '+' or '-' immediately after 'X'
        if (token[1] == '+' || token[1] == '-') {
            *is_incremental = true;
        }
        *xVal = strtof(token + 1, &endptr) * STEPS_PER_MM;  // Convert the number after 'X' to float
    } else {
        *xVal = 0;
    }
    sys.target.pos = *xVal;
    // Find the 'F' value
    token = strchr(copy, 'F');
    if (token != NULL) {
        *fVal = strtof(token + 1, &endptr) * STEPS_PER_MM;  // Convert the number after 'F' to float
    } else {
        *fVal = MAX_FEED_RATE * STEPS_PER_MM / 2;
    }

    // Find the 'A' value
    token = strchr(copy, 'A');
    if (token != NULL) {
        *aVal = strtof(token + 1, &endptr) * STEPS_PER_MM;  // Convert the number after 'A' to float
    } else {
        *aVal = MAX_ACCEL * STEPS_PER_MM / 2;
    }
    free(copy);  // Free the copied string
}

void homing(void){
    uint32_t symbol_duration = 0;
    rmt_symbol_word_t symbol;
    int phase = 0;
    symbol_duration = STEP_MOTOR_RESOLUTION_HZ * STEPS_PER_MM / HOMING_FAST_SPEED / 2;
    symbol.duration0 = symbol_duration;
    symbol.level0 = 0;
    symbol.duration1 = symbol_duration;
    symbol.level1 = 1;
    gpio_set_level(STEP_MOTOR_GPIO_DIR, STEP_MOTOR_SPIN_DIR_COUNTERCLOCKWISE);
    
    while(1) {
        if (phase == 0) {
            if(gpio_get_level(LIMIT_SWITCH_MIN_GPIO)){
                rmt_transmit(motor_chan, stepper_encoder, &symbol, sizeof(rmt_symbol_word_t), &tx_config);
            } else {
                phase = 1;
                gpio_set_level(STEP_MOTOR_GPIO_DIR, STEP_MOTOR_SPIN_DIR_CLOCKWISE);       
            }
        } else if (phase == 1) {
            for (int i = 0; i < (int)(HOMING_RETRACTION_DISTANCE * STEPS_PER_MM); i++) {
                rmt_transmit(motor_chan, stepper_encoder, &symbol, sizeof(rmt_symbol_word_t), &tx_config);
            }
            phase = 2;
            gpio_set_level(STEP_MOTOR_GPIO_DIR, STEP_MOTOR_SPIN_DIR_COUNTERCLOCKWISE);
            symbol_duration = STEP_MOTOR_RESOLUTION_HZ * STEPS_PER_MM / HOMING_SLOW_SPEED / 2;
            symbol.duration0 = symbol_duration;
            symbol.level0 = 0;
            symbol.duration1 = symbol_duration;
            symbol.level1 = 1;
        } else if (phase == 2) {
            if(gpio_get_level(LIMIT_SWITCH_MIN_GPIO)) {
                rmt_transmit(motor_chan, stepper_encoder, &symbol, sizeof(rmt_symbol_word_t), &tx_config);
            } else {
                phase = 3;
                gpio_set_level(STEP_MOTOR_GPIO_DIR, STEP_MOTOR_SPIN_DIR_CLOCKWISE);
                symbol_duration = STEP_MOTOR_RESOLUTION_HZ * STEPS_PER_MM / HOMING_FAST_SPEED / 2;
                symbol.duration0 = symbol_duration;
                symbol.level0 = 0;
                symbol.duration1 = symbol_duration;
                symbol.level1 = 1;
            }
        } else if (phase == 3) {
            for (int i = 0; i < (int)(HOMING_RETRACTION_DISTANCE * STEPS_PER_MM); i++) {
                rmt_transmit(motor_chan, stepper_encoder, &symbol, sizeof(rmt_symbol_word_t), &tx_config);
            }
            phase = 4;
        } else if (phase == 4) {
            sys.state = STATE_IDLE;
            return;
        }
    }
}