#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "driver/gpio.h"
#include <string.h>
#include "esp_log.h"

#include "nvs_functions.h"

static const char *TAG = "nvs_own_functions";

void init_nvs(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );   
}

/* Store in NVS the name string
   that has been passed as a variable.
   Return an error if anything goes wrong
   during this process.
 */
esp_err_t store_string_in_nvs_with_error(char* variable_namespace, char* variable_name, char* variable_value)
{
    nvs_handle_t my_handle;
    esp_err_t err;

    // Open
    err = nvs_open(variable_namespace, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) return err;

    // Write variable_value
    int32_t required_size = strlen(variable_value) + 1; // include the null terminator
    err = nvs_set_blob(my_handle, variable_name, variable_value, required_size);
    if (err != ESP_OK) {
        nvs_close(my_handle);
        return err;
    }

    // Commit
    err = nvs_commit(my_handle);
    if (err != ESP_OK) {
        nvs_close(my_handle);
        return err;
    }

    // Close
    nvs_close(my_handle);
    return ESP_OK;
}

void store_string_in_nvs(char* variable_namespace, char* variable_name, char* variable_value)
{
    esp_err_t err;
    err = store_string_in_nvs_with_error(variable_namespace, variable_name, variable_value);
    if (err != ESP_OK) ESP_LOGE(TAG,"Error (%s) saving variable_value to NVS!\n", esp_err_to_name(err)); 
}

esp_err_t log_as_info_a_string_value_from_nvs_with_error(char* variable_namespace, char* variable_name)
{
    nvs_handle_t my_handle;
    esp_err_t err;

    // Open
    err = nvs_open(variable_namespace, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) return err;

    // Read variable_value
    size_t required_size = 0;  // value will default to 0, if not set yet in NVS
    // obtain required memory space to store blob being read from NVS
    err = nvs_get_blob(my_handle, variable_name, NULL, &required_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;
    
    if (required_size == 0) {
        ESP_LOGI(TAG, "NOTHING SAVED YET for the variable %s from NVS namespace %s", variable_name, variable_namespace);
    } else {
        char* variable_value = malloc(required_size);
        err = nvs_get_blob(my_handle, variable_name, variable_value, &required_size);
        if (err != ESP_OK) {
            free(variable_value);
            return err;
        }
        ESP_LOGI(TAG, "Value of variable %s from NVS namespace %s: %s", variable_name, variable_namespace,  variable_value);

        free(variable_value);
    }

    // Close
    nvs_close(my_handle);
    return ESP_OK;
}

void log_as_info_a_string_value_from_nvs(char* variable_namespace, char* variable_name)
{
    esp_err_t err;
    err = log_as_info_a_string_value_from_nvs_with_error(variable_namespace, variable_name);
    if (err != ESP_OK) ESP_LOGE(TAG,"Error (%s) PRINTing variable from NVS!\n", esp_err_to_name(err)); 
}


esp_err_t get_string_value_from_nvs_with_error(char* variable_namespace, char* variable_name, char** variable_value)
{
    nvs_handle_t my_handle;
    esp_err_t err;

    // Open
    err = nvs_open(variable_namespace, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) return err;

    // Read variable_value
    size_t required_size = 0;  // value will default to 0, if not set yet in NVS
    // obtain required memory space to store blob being read from NVS
    err = nvs_get_blob(my_handle, variable_name, NULL, &required_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        nvs_close(my_handle);
        return err;
    }
    
    if (required_size == 0) {
        ESP_LOGI(TAG, "NOTHING SAVED YET for the variable %s from NVS namespace %s", variable_name, variable_namespace);
        nvs_close(my_handle);
        return ESP_OK;
    } else {
        *variable_value = malloc(required_size + 1);
        err = nvs_get_blob(my_handle, variable_name, *variable_value, &required_size);
        if (err != ESP_OK) {
            free(*variable_value);
            nvs_close(my_handle);
            return err;
        }
        (*variable_value)[required_size] = '\0';
        ESP_LOGI(TAG, "Value of variable %s from NVS namespace %s: %s", variable_name, variable_namespace,  *variable_value);

        nvs_close(my_handle);
        return ESP_OK;
    }
}

char* get_string_value_from_nvs(char* variable_namespace, char* variable_name)
{
    esp_err_t err;
    char* variable_value = NULL;
    err = get_string_value_from_nvs_with_error(variable_namespace, variable_name, &variable_value);
    if (err != ESP_OK) {
        ESP_LOGE(TAG,"Error (%s) PRINTing variable from NVS!\n", esp_err_to_name(err));
        return NULL;
    }
    return variable_value;
}