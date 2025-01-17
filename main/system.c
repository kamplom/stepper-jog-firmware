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
#include "driver/pulse_cnt.h"
#include <errno.h>
// Include modules
#include "system.h"
#include "config.h"
#include "settings.h"
#include "wheel.h"
#include "serial.h"
#include "u_convert.h"
#include "limits.h"
#include "report.h"
#include "jogging.h"
#include "report.h"



static const char *TAG = "System";

rmt_transmit_config_t tx_config = {};
rmt_channel_handle_t motor_chan = NULL;
rmt_encoder_handle_t stepper_encoder = NULL;
QueueHandle_t uart_queue = NULL;
system_t sys = {};
pcnt_unit_handle_t pcnt_unit = NULL;
jog_aux_t jog_aux = {};


float convert_to_smooth_freq(uint32_t freq1, uint32_t freq2, uint32_t freqx)
{
    float normalize_x = ((float)(freqx - freq1)) / (freq2 - freq1);
    // third-order "smoothstep" function: https://esn.wikipedia.org/wiki/Smoothstep
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
        .gpio_num = settings.gpio.motor_step,
        .mem_block_symbols = settings.rmt.mem_block_sym,
        .resolution_hz = settings.rmt.motor_resolution,
        .trans_queue_depth = settings.rmt.queue_depth // set the number of transactions that can be pending in the background
    };
    ESP_LOGI(TAG, "Create RMT TX channel");
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &motor_chan));
    ESP_LOGI(TAG, "Enable RMT channel");
    ESP_ERROR_CHECK(rmt_enable(motor_chan));
}

void create_rmt_encoder(void)
{
    tx_config.loop_count = 0;
    tx_config.flags.queue_nonblocking = false;
    rmt_copy_encoder_config_t copy_encoder_config = {};
    ESP_ERROR_CHECK(rmt_new_copy_encoder(&copy_encoder_config, &stepper_encoder));
}

bool string_is_empty(const char *str)
{
    return str == NULL || str[0] == '\0';
}

void invert_hard_limit_pin(void)
{
    if (sys.hard_limit_pin == settings.gpio.limit_max)
    {
        sys.hard_limit_pin = settings.gpio.limit_min;
    }
    else
    {
        sys.hard_limit_pin = settings.gpio.limit_max;
    }
}

void invert_motor_direction(void)
{
    gpio_set_level(settings.gpio.motor_dir, !sys.status.dir);
    sys.status.dir = !sys.status.dir;
    vTaskDelay(pdMS_TO_TICKS(settings.motion.dir_delay));
    invert_hard_limit_pin();
}

void set_motor_direction(bool dir)
{
    // if (settings.motion.dir) {
    //     gpio_set_level(settings.gpio.motor_dir, dir);
    // } else {
    //     gpio_set_level(settings.gpio.motor_dir, !dir);
    // }
    sys.status.dir = dir;
    gpio_set_level(settings.gpio.motor_dir, dir);
    // vTaskDelay(pdMS_TO_TICKS(settings.motion.dir_delay));
}

bool str_to_u32(char *str, uint32_t *out)
{
    char *end_ptr = NULL;
    errno = 0;
    uint32_t number = strtoul(str, &end_ptr, 10);
    if (errno == ERANGE && number == UINT32_MAX)
    {
        return false; // Out of range for uint32_t
    }
    if (end_ptr == str || *end_ptr != '\0')
    {
        return false; // No valid conversion, or leftover characters
    }
    *out = (uint32_t)number;
    return true;
}

bool str_to_float(char *str, float *out)
{
    char *end_ptr = NULL;
    errno = 0;
    float number = strtof(str, &end_ptr);
    // ESP_LOGD(TAG, "str: %s", str);
    // ESP_LOGD(TAG, "float: %f", number);
    if (errno == ERANGE)
    {
        printf("Number out of range");
        return false;
    }
    else if (end_ptr == str || *end_ptr != '\0')
    {
        printf("leftover characters");
        return false;
    }
    *out = number;
    return true;
}

void parse_jog_command(const char *command, size_t len) {
    char *copy = strndup(command, len);
    //first thing is the X or Y for axis, then value
    uint32_t xVal = 0;
    xVal = mm_to_pulses(strtoul(copy + 1, NULL, 10));
    sys.target.pos = soft_limits_check(xVal);
    set_state(STATE_JOGGING);
}

void parse_set_setting(const char *command, size_t len) {
    char *copy = strndup(command, len);
    char *end_ptr;
    uint32_t id = strtoul(copy, &end_ptr, 10);
    if (*end_ptr == '=')
    {
        if (set_setting(id, end_ptr + 1))
        {
            report_setting_short(id);
            return;
        }
        else
        {
            printf("Setting not set correctly\n");
            return;
        }
    } else if (*end_ptr == '\0') {
        report_setting_short(id);
    } else {
        printf("Not a valid id\n");
        return;
    }

}

void parse_report_setting(const char *command, size_t len) {
    char *copy = strndup(command, len);
    uint32_t id;
    if (str_to_u32(copy, &id))
    {
        report_setting_short(id);
        return;
    }
    else
    {
        printf("Not a valid id\n");
        return;
    }
}

void parse_command(const char *command, size_t len) {
    char *copy = strndup(command, len);

    if (string_is_empty(copy)) {
        return;
    }
    switch(copy[0]){
        case JOG_CANCEL_COMMAND:
            cancel_jog();
            return;
            break;
        case '?':
            update_real_pos();
            report_position();
            return;
            break;
        case '$':
            switch(copy[1]) {
                case '$': //$$
                    if (copy[2] == '\0') {
                        report_all_long();
                        return;
                    } else {
                        uint32_t id;
                        // we are not checking if it is null terminated
                        if (str_to_u32(copy + 2, &id)) {
                            report_setting_long(id);
                            return;
                        } else {
                            printf("Not a valid id\n");
                            return;
                        }
                    }
                    return;
                    break;
                case 'J': //$J - maybe avoid sending the $? it is no longer grbl compatible
                    if (copy[2] == 'X') { //will add heere denominations for other axis
                        parse_jog_command(&copy[2], len - 2);
                        return;
                    } else {
                        return;
                    }
                    return;
                    break;
                case '\0': //$
                    report_all_short();
                    return;
                    break;
                default:
                    parse_set_setting(&copy[1], len - 1);
                    return;
                    break;
            }
        case HOMING_COMMAND:
            sys.state = STATE_HOMING;
            return;
            break;
        default:
            return;
            break;
    }
}

void update_real_pos() {
    int32_t count;
    pcnt_unit_get_count(pcnt_unit, &count);
    sys.real.pos = count + (int32_t)settings.homing.offset;
}

esp_err_t set_home_here(){
    pcnt_unit_clear_count(pcnt_unit);
    sys.real.pos = mm_to_pulses(settings.homing.offset);
    return ESP_OK;
}

void homing(void)
{
    uint32_t symbol_duration = 0;
    rmt_symbol_word_t symbol;
    int phase = 0;
    symbol_duration = settings.rmt.motor_resolution / settings.homing.fast_vel / 2;
    symbol.duration0 = symbol_duration;
    symbol.level0 = 0;
    symbol.duration1 = symbol_duration;
    symbol.level1 = 1;

    if (settings.homing.direction)
    {
        set_motor_direction(settings.motion.dir);
    }
    else
    {
        set_motor_direction(!settings.motion.dir);
    }
    ESP_LOGI(TAG, "Homing dir: %d", settings.homing.direction);
    ESP_LOGI(TAG, "motion dir: %d", settings.motion.dir);
    ESP_LOGI(TAG, "supposed value: %d", !(settings.motion.dir^settings.homing.direction));
    ESP_LOGI(TAG, "dir gpio: %d", gpio_get_level(settings.gpio.motor_dir));

    while (1)
    {
        if (phase == 0)
        {
            if (gpio_get_level(settings.gpio.limit_min))
            {
                rmt_transmit(motor_chan, stepper_encoder, &symbol, sizeof(rmt_symbol_word_t), &tx_config);
            }
            else
            {
                phase = 1;
                invert_motor_direction();
                symbol_duration = settings.rmt.motor_resolution / settings.homing.slow_vel / 2;
                symbol.duration0 = symbol_duration;
                symbol.duration1 = symbol_duration;
            }
        }
        else if (phase == 1)
        {
            for (int i = 0; i < settings.homing.retraction; i++)
            {
                rmt_transmit(motor_chan, stepper_encoder, &symbol, sizeof(rmt_symbol_word_t), &tx_config);
            }
            phase = 2;
            invert_motor_direction();
            symbol_duration = settings.rmt.motor_resolution / settings.homing.slow_vel / 2;
            symbol.duration0 = symbol_duration;
            symbol.duration1 = symbol_duration;
        }
        else if (phase == 2)
        {
            if (gpio_get_level(LIMIT_SWITCH_MIN_GPIO))
            {
                rmt_transmit(motor_chan, stepper_encoder, &symbol, sizeof(rmt_symbol_word_t), &tx_config);
            }
            else
            {
                phase = 3;
                invert_motor_direction();
                symbol_duration = settings.rmt.motor_resolution / settings.homing.slow_vel / 2;
                symbol.duration0 = symbol_duration;
                symbol.duration1 = symbol_duration;
            }
        }
        else if (phase == 3)
        {
            set_home_here();
            for (int i = 0; i < settings.homing.retraction; i++)
            {
                rmt_transmit(motor_chan, stepper_encoder, &symbol, sizeof(rmt_symbol_word_t), &tx_config);
            }
            phase = 4;
        }
        else if (phase == 4)
        {
            sys.state = STATE_IDLE;
            sys.status.pos = 0;
            sys.status.vel = 0;
            sys.status.acc = 0;
            return;
        }
    }
}

bool set_state(uint8_t state)
{
    // do not use sys.state = in this function
    // in order to not be able to overwrite the alert state
    switch (state)
    {
    case STATE_ALERT:
        // only overwrite if it is the alert case
        sys.state = STATE_ALERT;
        ESP_LOGD(TAG, "State: alert");
        return true;
    case STATE_HOMING:
        if (sys.state & (STATE_ALERT | STATE_IDLE))
        {
            // clear states
            sys.state &= ~(STATE_ALERT | STATE_IDLE);
            // set state wihtout overwritting
            sys.state |= STATE_HOMING;
            ESP_LOGD(TAG, "State homing");
            return true;
        }
        else
        {
            return false;
        }
    case STATE_JOGGING:
        if (sys.state & STATE_IDLE)
        {
            sys.state &= ~STATE_IDLE;
            sys.state |= STATE_JOGGING;
            ESP_LOGD(TAG, "State: jogging");
            return true;
        }
        else
        {
            return false;
        }
    case STATE_WHEEL:
        if (sys.state & STATE_IDLE)
        {
            sys.state &= ~STATE_IDLE;
            sys.state |= STATE_WHEEL;
            ESP_LOGD(TAG, "State: wheel");
            return true;
        }
        else
        {
            return false;
        }
    case STATE_IDLE:
        if (sys.state & (STATE_JOGGING | STATE_HOMING | STATE_WHEEL))
        {
            sys.state &= ~(STATE_JOGGING | STATE_HOMING | STATE_WHEEL);
            sys.state |= STATE_IDLE;
            ESP_LOGD(TAG, "State: idle");
            return true;
        }
        else
        {
            return false;
        }
    case STATE_MLOCKED:
        if (settings.motion.lock) { 
            sys.state |= STATE_MLOCKED;
        } else {
            sys.state &= ~STATE_MLOCKED;
        }
        
    default:
        return false;
    }
    // just in case we manage to get here somehow.
    return false;
}

bool motor_enabler(bool action)
{
    if (sys.state & STATE_ALERT)
    {
        ESP_LOGD(TAG, "Motor not enabled, alert state");
        gpio_set_level(STEP_MOTOR_GPIO_EN, !STEP_MOTOR_ENABLE_LEVEL);
        return false;
    }
    if (action)
    {
        gpio_set_level(STEP_MOTOR_GPIO_EN, STEP_MOTOR_ENABLE_LEVEL);
        vTaskDelay(pdMS_TO_TICKS(ENABLE_DELAY));
        ESP_LOGD(TAG, "Motor enabled");
        return true;
    }
    else if (!settings.motion.lock)
    {
        gpio_set_level(STEP_MOTOR_GPIO_EN, !STEP_MOTOR_ENABLE_LEVEL);
        vTaskDelay(pdMS_TO_TICKS(ENABLE_DELAY));
        ESP_LOGD(TAG, "Motor disabled");
        return true;
    }
    return false;
}

esp_err_t start_up_sequence()
{
    // temporal
    ESP_LOGI(TAG, "Initialize EN + DIR GPIO");
    gpio_config_t en_dir_gpio_config = {
        .mode = GPIO_MODE_INPUT_OUTPUT,
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
    settings_init();
    // end of temporal
    //  init rmt channel and encoder
    create_rmt_channel();
    create_rmt_encoder();

    init_uart();

    // init sys variables
    sys.target.pos = 0;
    sys.status.pos = 0;
    sys.status.vel = 0;
    sys.state = STATE_ALERT;

    // main loop. If target is not pos exectues whatever update_velocity tells it to.
    pcnt_init();
    // wheel_timer_init();
    pos_report_timer_init();
    status_report_timer_init();
    return ESP_OK;
}