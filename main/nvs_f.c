// include esp-idf libraries
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

esp_err_t nvs_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    return err;
}
esp_err_t nvs_write_setting_helper(uint32_t index) {
    esp_err_t err;
    switch (setting_detail[index].datatype)
    {
        case Format_Int:
            err = nvs_set_u32(nvs_settings, setting_detail[index].key, *(uint32_t *)setting_detail[index].value);
            break;
        case Format_Float:
            err = ESP_FAIL;
            break;
        case Format_Bool:
            err = nvs_set_u8(nvs_settings, setting_detail[index].key, *(bool *)setting_detail[index].value ? 1 : 0);
            break;
        default:
            err = ESP_FAIL;
            break;
    }
    return err;
}

esp_err_t nvs_write_setting(uint32_t id)
{
    uint32_t index;
    esp_err_t err;
    esp_err_t err_write;
    //find setting
    if (!find_setting(id, &index))
    {
        printf("Setting not found\n");
        return ESP_FAIL;
    }
    //open nvs
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvs_settings);
    if (err != ESP_OK)
    {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return ESP_FAIL;
    }
    //write setting
    err_write = nvs_write_setting_helper(index);
    printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
    //commit and close
    err = nvs_commit(nvs_settings);
    printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
    nvs_close(nvs_settings);
    if (err != ESP_OK || err_write != ESP_OK)
    {
        return ESP_FAIL;
    } else {
        return ESP_OK;
    }
}

esp_err_t nvs_write_all_settings(void)
{
    esp_err_t err;
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvs_settings);
    if (err != ESP_OK)
    {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return ESP_FAIL;
    }

    for (uint32_t index = 0; index < N_settings; index++)
    {
        nvs_write_setting_helper(index);
    }
    //commit and close
    err = nvs_commit(nvs_settings);
    printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
    nvs_close(nvs_settings);
    if (err != ESP_OK)
    {
        return ESP_FAIL;
    } else {
        return ESP_OK;
    }
}

esp_err_t nvs_read_setting_helper(uint32_t index) {
    esp_err_t err;
    switch (setting_detail[index].datatype)
    {
        case Format_Int:
            err = nvs_get_u32(nvs_settings, setting_detail[index].key, (uint32_t *)setting_detail[index].value);
            break;
        case Format_Float:
            err = ESP_FAIL;
            break;
        case Format_Bool:
            uint8_t bool_val;
            err = nvs_get_u8(nvs_settings, setting_detail[index].key, &bool_val);
            if (err == ESP_OK) {
                *(bool *)setting_detail[index].value = bool_val ? true : false;
            }
            break;
        default:
            err = ESP_FAIL;
            break;
    }
    if (err != ESP_OK)
    {
        printf("Error (%s) reading. Setting ID:%"PRIu16, esp_err_to_name(err), setting_detail[index].id);
    }
    return err;
}

esp_err_t nvs_read_setting(uint32_t id)
{
    uint32_t index;
    if (!find_setting(id, &index))
    {
        printf("setting not found\n");
        return ESP_FAIL;
    }
    esp_err_t err;
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvs_settings);
    if (err != ESP_OK)
    {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return ESP_FAIL;
    }

    err = nvs_read_setting_helper(index);

    nvs_close(nvs_settings);
    return ESP_OK;
}

esp_err_t nvs_read_all_settings(void)
{
    ESP_LOGI(TAG, "Reading all settings");
    esp_err_t err;
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvs_settings);
    if (err != ESP_OK)
    {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return ESP_FAIL;
    }

    for (uint32_t index = 0; index < N_settings; index++)
    {
        err = nvs_read_setting_helper(index);
        if (err != ESP_OK)
        {
            printf("Error (%s) reading. Setting ID:%"PRIu16"\n", esp_err_to_name(err), setting_detail[index].id);
        } else {
            report_setting_short(index);
        }

    }
    nvs_close(nvs_settings);
    return ESP_OK;
}