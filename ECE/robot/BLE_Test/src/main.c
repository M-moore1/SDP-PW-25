#include "Robot_BLE.h"
#include "stepper_motor.h"
#include "pinout.h"
#include "nvs_flash.h"
#include "esp_log.h"

step_mot_t test_motor;

static QueueHandle_t cmd_queue = NULL;

// Type "FORWARD" to move the motor direction 0 for 1 seconds
// Type "BACKWARD" to move the motor direction 1 for 1 second
void ble_recieve_parser(void *pvParameters)
{
    uint8_t received_packet[156]; 
    char *msg1 = "got it!";

    while (1) {
        if (xQueueReceive(ble_recieve_queue, &received_packet, portMAX_DELAY))
        {   
            send_string(msg1);
            printf("Received 156 bytes: \n");

            for (int i = 0; i < 156; i++) {
                printf("%02X ", received_packet[i]);
                if ((i + 1) % 16 == 0) printf("\n"); 
            }
            printf("\n");
            printf("End of Bytes");
          
        }
    }
}

void command_parser(void *pvParameters)
{
    uint64_t inst;
    while (1)
    {
        if (xQueueReceive(cmd_queue, &inst, portMAX_DELAY))
        {
            printf("Command Parse Initiated \n");
            // Skip 2 then keep wherever a 1 is
            uint8_t type = (inst >> 2) & 0x1F;
            ESP_LOGI("CMD", "Extracted type: %u", type);

            switch (type){
                case CONTROL_CMD:
                    printf("I got a control instruction");
                break;

                case POSE_CMD:
                    printf("I got a Pose instruction");
                break;

                case System_CMD:
                    printf("I got a System instruction");
                break;
                
                case Query_CMD:
                    printf("I got a Query Instruction");
                break;
                
                default:
                    printf("Invalid Command");
                break;
            }


            
        }
    }
}

void app_main()
{
    
    ESP_ERROR_CHECK(nvs_flash_init()); // Initialize NVS
    robot_ble_init();                  // Initialize BLE
    cmd_queue = xQueueCreate(10, sizeof(uint64_t)); // Initialize the command queue
    
    char *msg1 = "hello";
    char *msg2 = "testing";

    motor_init(&test_motor, TEST_MOTOR_STEP, TEST_MOTOR_DIR, TEST_MOTOR_EN, TEST_MOTOR_CHANNEL);


    xTaskCreatePinnedToCore( ble_recieve_parser, "ble_recieve_parser", 4096, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore( command_parser, "robot_command_parser", 4096, NULL, 5, NULL, 1);

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