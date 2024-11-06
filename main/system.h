// Inlcude esp-idf libraries
#include "freertos/ringbuf.h"
#include "driver/rmt_tx.h"
#include "driver/uart.h"
#include "freertos/ringbuf.h"
#include "driver/pulse_cnt.h"

// Include moduels
#include "config.h"

#pragma once

#define LINE_ENDING '\n'
#define CCW         false
#define CW          true
typedef struct {
    uint32_t pos;
    int32_t vel;
    uint32_t acc;
    uint32_t jerk;
    bool dir;
} physical_state_t;

typedef struct {
    uint32_t pos;
    uint32_t vel;
    uint32_t acc;
    bool is_incremental;
} physical_target_t;

typedef struct {
    float vel;
} wheel_state_t;

typedef struct system {
    physical_state_t status;
    physical_state_t prev_status;
    physical_target_t target;
    uint8_t state;
    wheel_state_t wheel;    
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
extern pcnt_unit_handle_t pcnt_unit;

void create_rmt_channel(void);
void create_rmt_encoder(void);
void execute_line(char *payload, char *pattern);
float convert_to_smooth_freq(uint32_t freq1, uint32_t freq2, uint32_t freqx);
void SmoothDamp(void);
void parse_command(const char *command, uint32_t *xVal, uint32_t *fVal, uint32_t *aVal, bool *is_incremental);
void homing(void);
bool set_state(uint8_t state);
bool motor_enabler(bool action);

void mm_to_steps(float *mm, uint32_t *steps);
float steps_to_mm(uint32_t steps);

bool str_to_u32(char *str, uint32_t *out);
bool str_to_float(char *str, float *out);

void invert_motor_direction(void);
void set_motor_direction(bool dir);