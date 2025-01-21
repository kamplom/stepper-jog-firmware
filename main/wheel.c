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
#include "settings.h"


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
        .edge_gpio_num = settings.gpio.wheel_A,
        .level_gpio_num = settings.gpio.wheel_B,
    };
    pcnt_channel_handle_t pcnt_chan_a = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_a_config, &pcnt_chan_a));

    pcnt_chan_config_t chan_b_config = {
        .edge_gpio_num = settings.gpio.wheel_B,
        .level_gpio_num = settings.gpio.wheel_A,
    };
    pcnt_channel_handle_t pcnt_chan_b = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_b_config, &pcnt_chan_b));

    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcnt_unit, 10000));
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcnt_unit, -10000));

    ESP_LOGI(TAG, "enable pcnt unit");
    ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
    ESP_LOGI(TAG, "clear pcnt unit");
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
    ESP_LOGI(TAG, "start pcnt unit");
    ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));
}
void smooth_damp(float target, float *vel_ptr, float *acc_ptr) {
    float current = *(vel_ptr);
    float current_velocity = *(acc_ptr);
    float smooth_time = 0.3;
    float max_speed = 9999999999999999; //max accel
    float delta_time = fabs(1/(current+0.001));

    float omega = 2 / smooth_time;
    float x = omega * delta_time;
    float exp = 1 / (1 + x + 0.48f * x * x + 0.235f * x * x * x);
    float change = current - target;
    float originalTo = target;

    float max_change = max_speed * smooth_time;
    change = fmin(max_change, fmax(change, -max_change));
    target = current - change;

    float temp = (current_velocity + omega * change) * delta_time;
    float acc = (current_velocity - omega * temp) * exp;
    float vel = target + (change + temp) * exp;

    if ((originalTo - current > 0.0f) == (vel > originalTo)) {
        vel = originalTo;
        acc = (vel - originalTo) / delta_time;
    }
    if(vel < MIN_FEED_RATE  && vel > 0 && target == 0) {
        vel = 0;
    } else if (vel > -MIN_FEED_RATE && vel < 0 && target == 0) {
        vel = 0;
    }
    *(vel_ptr) = vel;
    *(acc_ptr) = acc;
}
static void wheel_timer_callback(void* arg) {
    int32_t pulse_count = 0;

    ESP_ERROR_CHECK(pcnt_unit_get_count(pcnt_unit, &pulse_count));
    pcnt_unit_clear_count(pcnt_unit);
    if (abs(pulse_count) > 60) {
        sys.wheel.vel = (pulse_count) * 1000 / WHEEL_TIMER_INTERVAL * MAX_FEED_RATE * STEPS_PER_MM / 10000;
    } else {
        sys.wheel.vel = 0;
    }
    
    //ESP_LOGI(TAG, "pulse count: %"PRIi32, pulse_count);

    if(sys.wheel.vel != 0) {
        set_state(STATE_WHEEL);
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

