#include "Robot_BLE.h"
#include "nvs_flash.h"

// IF NOTIFY NOT ENABLED SEND BACK info to ROBOT
// But everthing in BLE section

void app_main()
{
    
    ESP_ERROR_CHECK(nvs_flash_init()); // Initialize NVS
    robot_ble_init();                  // Initialize BLE
    
    char *msg1 = "hello";
    char *msg2 = "testing";

    while (1) {

        if (device_connected && notify_enabled) {

            

            send_string(msg2);  

            vTaskDelay(pdMS_TO_TICKS(200));

            send_string(msg1);

        }

        vTaskDelay(pdMS_TO_TICKS(2000)); // 2 seconds
    }
    return;
}