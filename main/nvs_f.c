//include esp-idf libraries
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <stdio.h>

// inlcude modules

#include "settings.h"
#include "nvs_f.h"

static const char *TAG = "NVS";

nvs_handle_t nvs_settings;

esp_err_t nvs_init(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    return err;
}

esp_err_t nvs_write_setting(uint32_t id) {
    uint32_t index;
    if (!find_setting(id, &index)) {
        printf("Setting not found\n");
        return ESP_FAIL;
    }
    esp_err_t err;
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvs_settings);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return ESP_FAIL;
    }
    err = nvs_set_u32(nvs_settings, setting_detail[index].key, *(uint32_t*)setting_detail[index].value);
    printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

    err = nvs_commit(nvs_settings);
    printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
    nvs_close(nvs_settings);
    return ESP_OK;
}

esp_err_t nvs_read_setting(uint32_t id) {
    uint32_t index;
    if (!find_setting(id, &index)) {
        printf("setting not found\n");
        return ESP_FAIL;
    }
    esp_err_t err;
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvs_settings);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return ESP_FAIL;
    }
    err = nvs_get_u32(nvs_settings, setting_detail[index].key, *(uint32_t*)setting_detail[index].value);

    nvs_close(nvs_settings);
    if (setting_detail[index].id == Setting_Stepsmm) {
        settings.motion.steps_mm = fixed_to_float(settings.motion.fixedp_steps_mm);
    }
    return ESP_OK;
}

esp_err_t nvs_read_all_settings(void) {
    esp_err_t err;
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvs_settings);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return ESP_FAIL;
    }

    for (uint32_t index = 0; index < N_settings; index++) {
        err = nvs_get_u32(nvs_settings, setting_detail[index].key, (uint32_t*)setting_detail[index].value);
    }
    nvs_close(nvs_settings);
    settings.motion.steps_mm = fixed_to_float(settings.motion.fixedp_steps_mm);
    return ESP_OK;
}

esp_err_t nvs_write_all_settings(void) {
    esp_err_t err;
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvs_settings);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return ESP_FAIL;
    }

    for (uint32_t index = 0; index < N_settings; index++) {
        err = nvs_set_u32(nvs_settings, setting_detail[index].key, (uint32_t*)setting_detail[index].value);
    }
    nvs_close(nvs_settings);
    return ESP_OK;
}