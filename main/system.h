#include "freertos/ringbuf.h"
#include "driver/rmt_tx.h"
#include "config.h"
#include "driver/uart.h"
#include "freertos/ringbuf.h"

#pragma once

#define LINE_ENDING '\n'

typedef struct {
    uint32_t pos;
    float vel;
    uint32_t acc;
} physical_state_t;

typedef struct {
    uint32_t pos;
    uint32_t vel;
    uint32_t acc;
    bool is_incremental;
    uint32_t accel_steps;
    uint32_t constant_speed_steps;
    uint32_t total_steps;
    bool dir;
} physical_target_t;

typedef struct system {
    bool position_lost;
    uint32_t position;
    physical_state_t status;
    physical_target_t target;
} system_t;


typedef struct {
    RingbufHandle_t buff1;
    RingbufHandle_t buff2;
    uint8_t active_buffer;
} stepper_encode_buffers_t;



extern system_t sys;
extern rmt_transmit_config_t tx_config;
extern rmt_channel_handle_t motor_chan;
extern rmt_encoder_handle_t stepper_encoder;
extern QueueHandle_t uart_queue;
extern RingbufHandle_t line_buff;


void create_rmt_channel(void);
void create_rmt_encoder(void);
void execute_line(char *payload, char *pattern);
float convert_to_smooth_freq(uint32_t freq1, uint32_t freq2, uint32_t freqx);
void SmoothDamp(void);