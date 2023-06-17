#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_smartconfig.h"

#include "wifi_functions.h"
#include "nvs_functions.h"

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
static const int CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1;
static const char *TAG = "smartconfig_example";

static void smartconfig_example_task(void * parm);



// function that takes ssid and password as arguments and sets 
// the Wi-Fi configuration and connects to the access point
esp_err_t wifi_connect_with_parameters(char* ssid, char* password)
{
    esp_err_t err;
    wifi_config_t wifi_config;
    // Initialize the wifi_config structure with zero values
    bzero(&wifi_config, sizeof(wifi_config_t));
    // Copy the ssid and password to the wifi_config structure
    memcpy(wifi_config.sta.ssid, ssid, strlen(ssid) + 1); 
    memcpy(wifi_config.sta.password, password, strlen(password) + 1); 
    // Set the bssid_set flag to false, which means the device 
    // will not check the MAC address of the access point
    wifi_config.sta.bssid_set = false;

    // Log the ssid and password
    ESP_LOGI(TAG, "SSID:%s", ssid);
    ESP_LOGI(TAG, "PASSWORD:%s", password);

    // Disconnect from any previous Wi-Fi network
    ESP_ERROR_CHECK( esp_wifi_disconnect() );
    // Set the Wi-Fi configuration for the station interface
    ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    // Connect to the access point
    err = esp_wifi_connect();
    return err;
}

esp_err_t wifi_connect_from_nvs()
{
    esp_err_t err = ESP_FAIL;
    char ssid[33] = { 0 };
    char password[65] = { 0 };
    char* ssid_value = get_string_value_from_nvs("x", "SSID");
    char* password_value = get_string_value_from_nvs("x", "psswd");
    if (ssid_value != NULL && password_value != NULL) {
        strcpy(ssid, ssid_value);
        strcpy(password, password_value);
        err = wifi_connect_with_parameters(ssid, password);
    }
    return err;
}
static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    esp_err_t err = ESP_FAIL;
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        xTaskCreate(smartconfig_example_task, "smartconfig_example_task", 4096, NULL, 3, NULL);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_connect_from_nvs();
        xEventGroupClearBits(s_wifi_event_group, CONNECTED_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        // Get the IP address from the event data
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        // Log an informational message with the IP address
        ESP_LOGI(TAG, "Wi-Fi connected with IP: " IPSTR, IP2STR(&event->ip_info.ip));
        // Set the CONNECTED_BIT in the event group to indicate Wi-Fi connection
        xEventGroupSetBits(s_wifi_event_group, CONNECTED_BIT);
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE) {
        ESP_LOGI(TAG, "Scan done");
        err = wifi_connect_from_nvs();
        if (err == ESP_OK){ 
        xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
        }
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL) {
        ESP_LOGI(TAG, "Found channel");        
    } else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD) {
        ESP_LOGI(TAG, "Got SSID and password");
        smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
        
        // Declare and initialize the variables with the evt->ssid and evt->password
        char ssid[33] = { 0 };
        char password[65] = { 0 };
        strcpy(ssid, (char*)evt->ssid);
        strcpy(password, (char*)evt->password);

        err = wifi_connect_with_parameters(ssid, password);

        if (err == ESP_OK){ 
            store_string_in_nvs("x", "SSID", ssid);
            store_string_in_nvs("x", "psswd", password);
        }

    } else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE) {
        xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
    }
}

void init_wifi(void)
{
    // Initialize the TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());

    // Create an event group to handle Wi-Fi events
    s_wifi_event_group = xEventGroupCreate();

    // Create the default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create a default Wi-Fi station interface and get its handle
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();

    // Check that the interface was created successfully
    assert(sta_netif);

    // Initialize the Wi-Fi driver with the default configuration
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );

    // Register a callback function to handle Wi-Fi events
    ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );

    // Register a callback function to handle IP events, such as getting an IP address from a DHCP server
    ESP_ERROR_CHECK( esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL) );

    // Register a callback function to handle smartconfig events
    ESP_ERROR_CHECK( esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );

    // Set the Wi-Fi mode to station mode, which means the device will connect to an access point
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );

    // Start the Wi-Fi driver

    ESP_ERROR_CHECK( esp_wifi_start() );
}


static void smartconfig_example_task(void * parm)
{
    EventBits_t uxBits;
    ESP_ERROR_CHECK( esp_smartconfig_set_type(SC_TYPE_ESPTOUCH) );
    smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_smartconfig_start(&cfg) );
    while (1) {
        uxBits = xEventGroupWaitBits(s_wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY);
        if(uxBits & CONNECTED_BIT) {
            ESP_LOGI(TAG, "WiFi Connected to ap");
        }
        if(uxBits & ESPTOUCH_DONE_BIT) {
            ESP_LOGI(TAG, "smartconfig over");
            esp_smartconfig_stop();
            vTaskDelete(NULL);
        }
    }
}