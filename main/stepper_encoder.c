/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "esp_check.h"
#include "stepper_encoder.h"

typedef struct {
    rmt_encoder_t base;
    rmt_encoder_t *copy_encoder;
} rmt_stepper_encoder_t;

static const char *TAG = "stepper_motor_encoder";

static esp_err_t rmt_stepper_encoder_del(rmt_encoder_t *encoder)
{
    rmt_stepper_encoder_t *motor_encoder = __containerof(encoder, rmt_stepper_encoder_t, base);
    rmt_del_encoder(motor_encoder->copy_encoder);
    free(motor_encoder);
    return ESP_OK;
}

static esp_err_t rmt_stepper_encoder_reset(rmt_encoder_t *encoder)
{
    rmt_stepper_encoder_t *motor_encoder = __containerof(encoder, rmt_stepper_encoder_t, base);
    rmt_encoder_reset(motor_encoder->copy_encoder);
    return ESP_OK;
}

static size_t rmt_encode_stepper(rmt_encoder_t *encoder, rmt_channel_handle_t channel, const void *primary_data, size_t data_size, rmt_encode_state_t *ret_state)
{
    return ESP_OK;
}

esp_err_t rmt_new_stepper_encoder(const stepper_encoder_config_t *config, rmt_encoder_handle_t *ret_encoder)
{
    esp_err_t ret = ESP_OK;
    rmt_stepper_encoder_t *stepper_encoder = NULL;
    ESP_GOTO_ON_FALSE(config && ret_encoder, ESP_ERR_INVALID_ARG, err, TAG, "invalid argument");
    stepper_encoder = rmt_alloc_encoder_mem(sizeof(rmt_stepper_encoder_t));
    ESP_GOTO_ON_FALSE(stepper_encoder, ESP_ERR_NO_MEM, err, TAG, "no mem for ir nec encoder");
    stepper_encoder->base.encode = rmt_encode_stepper;
    stepper_encoder->base.del = rmt_stepper_encoder_del;
    stepper_encoder->base.reset = rmt_stepper_encoder_reset;

    rmt_copy_encoder_config_t copy_encoder_config = {};
    ESP_GOTO_ON_ERROR(rmt_new_copy_encoder(&copy_encoder_config, &stepper_encoder->copy_encoder), err, TAG, "create copy encoder failed");
    *ret_encoder = &(stepper_encoder->base);
    return ESP_OK;

err:
    if (stepper_encoder) {
        if (stepper_encoder->copy_encoder) {
            rmt_del_encoder(stepper_encoder->copy_encoder);
        }
        free(stepper_encoder);
    }
    return ret;
}