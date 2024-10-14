// esp-idf components
#include "driver/pulse_cnt.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/rmt.h"
#include <math.h>

// include modules
#include "config.h"
#include "system.h" 


static const char *TAG = "wheel";


void pcnt_init() {
    pcnt_unit_config_t unit_config =  {
        .high_limit = 10000,
        .low_limit = -10000,
        .flags.accum_count = 1,
    };
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));

    pcnt_glitch_filter_config_t fliter_config = {
        .max_glitch_ns = 1000,
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt_unit, &fliter_config));

    pcnt_chan_config_t chan_a_config = {
        .edge_gpio_num = WHEEL_ENCODER_A,
        .level_gpio_num = WHEEL_ENCODER_B,
    };
    pcnt_channel_handle_t pcnt_chan_a = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_a_config, &pcnt_chan_a));

    pcnt_chan_config_t chan_b_config = {
        .edge_gpio_num = WHEEL_ENCODER_B,
        .level_gpio_num = WHEEL_ENCODER_A,
    };
    pcnt_channel_handle_t pcnt_chan_b = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_b_config, &pcnt_chan_b));

    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

    ESP_LOGI(TAG, "enable pcnt unit");
    ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
    ESP_LOGI(TAG, "clear pcnt unit");
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
    ESP_LOGI(TAG, "start pcnt unit");
    ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));
}
void smooth_damp() {
    float smooth_time = 1;
    float omega = 2 / smooth_time;
    float delta_time = WHEEL_TIMER_INTERVAL / 1000; 

    float x = omega * delta_time;
    float exp = 1 / (1 + x + 0.48f * x * x + 0.235f * x * x * x);
    float change = sys.status.vel - sys.wheel.vel;
    float original_to = sys.wheel.vel;

    float temp = (sys.status.vel + omega * change) * delta_time;
    sys.status.vel = sys.wheel.vel + (change + temp) * exp;
    sys.status.vel = MAX_FEED_RATE * STEPS_PER_MM / 20000;

}
static void wheel_timer_callback(void* arg) {
    int32_t pulse_count = 0;
    static int16_t last_pulse_count = 0;

    ESP_ERROR_CHECK(pcnt_unit_get_count(pcnt_unit, &pulse_count));
    pcnt_unit_clear_count(pcnt_unit);
    sys.wheel.vel = (pulse_count) * 1000 / WHEEL_TIMER_INTERVAL;
    //ESP_LOGI(TAG, "wheel speed: %f", sys.wheel.vel);
    last_pulse_count = pulse_count;

    //smooth_damp();

    //sys.status.vel = sys.wheel.vel / 9000 * MAX_FEED_RATE * STEPS_PER_MM;
    //ESP_LOGI(TAG, "status vel: %f", sys.status.vel);
    float symbol_duration = fabs(STEP_MOTOR_RESOLUTION_HZ / sys.status.vel / 2);
    rmt_symbol_word_t symbol;
    symbol.duration0 = symbol_duration;
    symbol.level0 = 0;
    symbol.duration1 = symbol_duration;
    symbol.level1 = 1;
    rmt_transmit_config_t tx_config2;
    tx_config2.loop_count = -1;
    tx_config2.flags.queue_nonblocking = true;
    //if(sys.status.vel > 0) {
    //    gpio_set_level(STEP_MOTOR_GPIO_DIR, STEP_MOTOR_SPIN_DIR_COUNTERCLOCKWISE);
    //} else {
    //    gpio_set_level(STEP_MOTOR_GPIO_DIR, STEP_MOTOR_SPIN_DIR_CLOCKWISE);
    //}
    if(sys.wheel.vel != 0) {
        rmt_disable(motor_chan);
        rmt_enable(motor_chan);
        rmt_transmit(motor_chan, stepper_encoder, &symbol, sizeof(rmt_symbol_word_t), &tx_config2);
    } else {
        //sys.status.vel = 0;
        //rmt_disable(motor_chan);
        //rmt_enable(motor_chan);
    }

}

void wheel_timer_init() {
    const esp_timer_create_args_t wheel_timer_args = {
        .callback = &wheel_timer_callback,
        .name = "wheel_timer"
    };

    esp_timer_handle_t wheel_timer;
    ESP_ERROR_CHECK(esp_timer_create(&wheel_timer_args, &wheel_timer));

    ESP_ERROR_CHECK(esp_timer_start_periodic(wheel_timer, WHEEL_TIMER_INTERVAL*1000));
}

