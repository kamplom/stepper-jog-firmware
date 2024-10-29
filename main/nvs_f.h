#include "nvs.h"

#define STORAGE_NAMESPACE "storage"

extern nvs_handle_t nvs_settings;

esp_err_t nvs_init(void);

esp_err_t nvs_read_setting(uint32_t id);
esp_err_t nvs_write_setting(uint32_t id);
esp_err_t nvs_read_all_settings(void);
esp_err_t nvs_write_all_settings(void);