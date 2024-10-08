#include "config.h"
#include "driver/rmt_tx.h"
#include "esp_log.h"
#include "system.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <inttypes.h>
#include <math.h>
#include "driver/gpio.h"


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
void parseCommand(const char *command, uint32_t *xVal, uint32_t *fVal, uint32_t *aVal, bool *is_incremental) {
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


void SmoothDamp(void)
{
    // Based on Game Programming Gems 4 Chapter 1.10
    float smoothTime = 0.3;
    float target = sys.target.pos;
    float deltaTime = fabs(1/sys.status.vel);
    smoothTime = fmax(0.0001F, smoothTime);
    float omega = 2.0f / smoothTime;

    float x = omega * deltaTime;
    float exp = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);
    float change = sys.status.pos - target;
    float originalTo = target;
    float originalVelocity = sys.status.vel;
    float originalAcc = sys.status.acc;
    // Clamp maximum speed
    // float maxChange = MAX_FEED_RATE * STEPS_PER_MM * smoothTime;
    float maxChange = 1000 * STEPS_PER_MM * smoothTime;
    change = fmax(-maxChange, fmin(change, maxChange));
    target = sys.status.pos - change;

    float temp = (sys.status.vel + omega * change) * deltaTime;
    sys.status.vel = (sys.status.vel - omega * temp) * exp;
    
    sys.status.acc = ((sys.status.vel - originalVelocity))/deltaTime;
    float jerk = (sys.status.acc - originalAcc)/deltaTime;
    float max_jerk = 4500000;
    float diff = abs(sys.status.pos-originalTo);
    float maxSpeed2;
    // ESP_LOGI(TAG, "deltaTime: %f", deltaTime);
    // ESP_LOGI(TAG, "original_vel: %f", originalVelocity);
    if (diff < 60000) {
        maxSpeed2 =  5.14712684f/10000000000 * diff * diff * diff -7.61746767f/100000 * diff *diff + 3.65719245f * diff -3.32443566f * 1000 + 3325;
        // ESP_LOGI(TAG, "max speed2: %f", maxSpeed2);
    }
    else {
        maxSpeed2 = MAX_FEED_RATE;
    }
    max_jerk = fmin(max_jerk * ((fabs(originalVelocity)/maxSpeed2)*(fabs(originalVelocity)/maxSpeed2)+0.02), max_jerk);
    // ESP_LOGI(TAG, "max_jerk: %f", max_jerk);
    
    jerk = fmax(-max_jerk, fmin(jerk, max_jerk));
    // ESP_LOGI(TAG, "jerk: %f", jerk);
    
    sys.status.acc = originalAcc + jerk * deltaTime;
    // ESP_LOGI(TAG, "accel: %f", sys.status.acc);
    sys.status.vel = originalVelocity + sys.status.acc*deltaTime;
    // ESP_LOGI(TAG, "vel: %f", sys.status.vel);
    
    // Prevent overshooting
    // if (originalTo - sys.status.pos > 0.0F == output > originalTo)
    // {
    //     sys.status.vel = (output - originalTo) / deltaTime;
    // }

    if(sys.status.vel < 70 && sys.status.vel > 0) {
        sys.status.vel = 70;
    } else if (sys.status.vel > -70 && sys.status.vel < 0) {
        sys.status.vel = -70;
    }

    if (sys.status.vel > 0) {
        sys.status.pos += 1;
        gpio_set_level(STEP_MOTOR_GPIO_DIR, STEP_MOTOR_SPIN_DIR_CLOCKWISE);
    } else {
        sys.status.pos -= 1;
        gpio_set_level(STEP_MOTOR_GPIO_DIR, STEP_MOTOR_SPIN_DIR_COUNTERCLOCKWISE);
    }
}

void execute_line(char *payload, char *pattern) {
    // const char *command = "$J=X700F300A250";  // Example command $J=X00F700A700
    parseCommand(payload, &sys.target.pos, &sys.target.vel, &sys.target.acc, &sys.target.is_incremental);
    ESP_LOGI(TAG, "X: %"PRIu32", F: %"PRIu32", A: %"PRIu32", Is Incremental: %s", sys.target.pos, sys.target.vel, sys.target.acc, sys.target.is_incremental ? "true" : "false");

    uint32_t accel_steps = sys.target.vel * sys.target.vel / sys.target.acc;
    uint32_t total_steps;
    uint32_t constant_speed_steps;
    //simple case: we are stopped and we will end up stopped
    if (sys.target.pos > sys.status.pos) {
        total_steps = sys.target.pos - sys.status.pos;
        gpio_set_level(STEP_MOTOR_GPIO_DIR, STEP_MOTOR_SPIN_DIR_CLOCKWISE);
        sys.target.dir = true;
    } else {
        total_steps = sys.status.pos - sys.target.pos;
        gpio_set_level(STEP_MOTOR_GPIO_DIR, STEP_MOTOR_SPIN_DIR_COUNTERCLOCKWISE);
        sys.target.dir = false;
    }
    if (accel_steps * 2 > 0.8 * total_steps) {
        accel_steps = (uint32_t)(0.4 * total_steps);
        sys.target.vel = sqrt(sys.target.acc * accel_steps);
        constant_speed_steps = total_steps - 2 * accel_steps;
    } else {
        constant_speed_steps = total_steps - 2 * accel_steps;
    }
    sys.target.accel_steps = accel_steps;
    sys.target.constant_speed_steps = constant_speed_steps;
    sys.target.total_steps = total_steps;
    ESP_LOGI(TAG, "accel steps: %"PRIu32, sys.target.accel_steps);
    ESP_LOGI(TAG, "constant steps: %"PRIu32, sys.target.constant_speed_steps);
    ESP_LOGI(TAG, "total steps: %"PRIu32, sys.target.total_steps);
}