#include "Robot_BLE.h"
#include "stepper_motor.h"
#include "nvs_flash.h"

#define TEST_MOTOR_STEP GPIO_NUM_32
#define TEST_MOTOR_DIR  GPIO_NUM_33
#define TEST_MOTOR_EN   GPIO_NUM_25
#define TEST_MOTOR_CHANNEL LEDC_CHANNEL_0

step_mot_t test_motor;

// Type "FORWARD" to move the motor direction 0 for 1 seconds
// Type "BACKWARD" to move the motor direction 1 for 1 second
void cmd_parser_task(void *pvParameters)
{
    char *received_cmd;

    while (1) {
        if (xQueueReceive(cmd_queue, &received_cmd, portMAX_DELAY))
        {
            printf("\nInformation sent from BLE: %s\n", received_cmd);

            if(strcmp(received_cmd, "FORWARD") == 0){
                for(int i = 0; i < 17; i++){
                    motor_pulse(&test_motor, 50, 0);
                    vTaskDelay(pdMS_TO_TICKS(50));
                }
                
            }else if (strcmp(received_cmd, "BACKWARD") == 0){
                for(int i = 0; i < 17; i++){
                    motor_pulse(&test_motor, 50, 1);
                    vTaskDelay(pdMS_TO_TICKS(50));
                }
            }
            free(received_cmd);
        }
    }
}

void app_main()
{
    
    ESP_ERROR_CHECK(nvs_flash_init()); // Initialize NVS
    robot_ble_init();                  // Initialize BLE
    
    char *msg1 = "hello";
    char *msg2 = "testing";

    motor_init(&test_motor, TEST_MOTOR_STEP, TEST_MOTOR_DIR, TEST_MOTOR_EN, TEST_MOTOR_CHANNEL);


    xTaskCreatePinnedToCore(
        cmd_parser_task,
        "cmd_parser",
        4096,
        NULL,
        5,
        NULL,
        1
    );

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