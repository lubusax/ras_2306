/* Esptouch example

*/

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


void app_main(void)
{
    init_nvs(); // non-volatile storage 24k
    init_wifi(); // tries to connect to stored wifi, opens smartconfig through ESP-Touch app


}
