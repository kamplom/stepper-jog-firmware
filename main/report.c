#include <esp_log.h>
#include <esp_err.h>
#include "esp_timer.h"
#include <math.h>

#include "settings.h"
#include "u_convert.h"

esp_err_t report_bytes(const char *data, size_t len) {
    esp_err_t err = ESP_OK;
    // send to serial
    if (settings.stream.serial_activate) {
        uart_write_bytes(UART_NUM_0, data, len);
        //add \n at the end
        uart_write_bytes(UART_NUM_0, "\n", 1);
    }
    if (settings.stream.ws_activate) {
        //send to ws 
    }
    // send to ws    
    return err;
}

esp_err_t report_string(const char *data, size_t len) {
    esp_err_t err = ESP_OK;
    if (settings.stream.serial_activate) {
        uart_write_bytes(UART_NUM_0, data, len);
        uart_write_bytes(UART_NUM_0, "\n", 1);
    }
    if (settings.stream.ws_activate) {
        //send to ws 
    }
    return err;
}

esp_err_t report_status(){
    //craft the message
    //header >
    char msg[4];
    uint32_t mm = (uint32_t)round(pulses_to_mm(settings.motion.pos.max));
    msg[0] = '>';
    msg[1] = (char)sys.state;
    msg[2] = (mm >> 8) & 0xFF;
    msg[3] = mm & 0xFF;
    report_bytes(&msg[0], 4);
    return ESP_OK;
}

esp_err_t report_position(){
    //craft the message
    //header @
    uint32_t microm = pulses_to_microm(sys.real.pos);
    char msg[4];
    msg[0] = '@';
    msg[1] = (microm >> 16) & 0xFF;
    msg[2] = (microm >> 8) & 0xFF;
    msg[3] = microm & 0xFF;
    report_bytes(&msg[0], 4);
    return ESP_OK;
}

esp_err_t report_EoM(){
    //craft the message
    //header @
    char msg[1];
    msg[0] = '¬';
    report_bytes(&msg[0], 1);
    return ESP_OK;
}

//timers for periodic auto-reports

static void pos_report_timer_callback(void* arg) {
    int32_t last_pos = sys.real.pos;
    update_real_pos();
    if(sys.real.pos != last_pos) {
        report_position();
    }
    return;
}

esp_err_t pos_report_timer_init() {
    const esp_timer_create_args_t pos_report_timer_args = {
        .callback = &pos_report_timer_callback,
        .name = "pos_report_timer"
    };

    esp_timer_handle_t pos_report_timer;
    ESP_ERROR_CHECK(esp_timer_create(&pos_report_timer_args, &pos_report_timer));

    ESP_ERROR_CHECK(esp_timer_start_periodic(pos_report_timer, 25*1000));
    return ESP_OK;
}

static void status_report_timer_callback(void* arg) {
    report_status();
    return;
}

esp_err_t status_report_timer_init() {
    const esp_timer_create_args_t status_report_timer_args = {
        .callback = &status_report_timer_callback,
        .name = "status_report_timer"
    };

    esp_timer_handle_t status_report_timer;
    ESP_ERROR_CHECK(esp_timer_create(&status_report_timer_args, &status_report_timer));

    ESP_ERROR_CHECK(esp_timer_start_periodic(status_report_timer, 1500*1000));
    return ESP_OK;
}