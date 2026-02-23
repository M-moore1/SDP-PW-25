#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "stepper_motor.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "driver/uart.h"

#include "nvs_flash.h"
#include "esp_log.h"
#include "robot_bt.h"
#include "pinout.h"
#include "robot_commands.h"


step_mot_t test_motor;
static QueueHandle_t cmd_queue = NULL;


void bt_reciever_parser(void *pvParameters)
{
    uint64_t received_cmd;

    while (1) {
        if (xQueueReceive(bt_recieve_queue, &received_cmd, portMAX_DELAY)) {
            
            
            if(security_flag) {
                printf("\nDecrypting bits...");
                
            }

            // ADD PRIORITY

            // 3. Send the finalized packet to the main robot controller
            if (xQueueSend(cmd_queue, &received_cmd, 0) != pdPASS) {
                ESP_LOGW(TAG, "cmd_queue full");
            }
        }
    }
}


void command_parser(void *pvParameters)
{
    uint64_t inst;
    robot_bt_packet_t cmd;
    while (1)
    {
        if (xQueueReceive(cmd_queue, &inst, portMAX_DELAY))
        {
            printf("Command Parse Initiated \n");
            cmd.raw = inst;

            command_type_t cmd_type = (command_type_t)cmd.ctrl.type;
            ESP_LOGI("CMD", "Extracted type: %u", cmd_type);

            switch (cmd_type){
                case CONTROL_CMD:
                    printf("\nI got a control instruction\n");
                    control_cmd(cmd.ctrl, &test_motor, &test_motor, &test_motor, &test_motor);

                break;

                case POSE_CMD:
                    printf("\nI got a Pose instruction\n");
                    pose_cmd(inst, &test_motor, &test_motor, &test_motor, &test_motor);
                break;

                case System_CMD:
                    printf("\nI got a System instruction\n");
                    system_cmd(cmd.sys, &test_motor, &test_motor, &test_motor, &test_motor);
                break;
                
                case Query_CMD:
                    printf("\nI got a Query Instruction\n");
                    query_cmd(cmd.query, &test_motor, &test_motor, &test_motor, &test_motor);
                break;
                
                default:
                    ack_format_t ack = {0};
                    ack.pl = 1;
                    ack.type = ACK_CMD;       
                    ack.id = cmd.query.id;    
                    ack.result_code = RESULT_UNKNOWN_CMD;
                    printf("\nInvalid Command\n");
                break;
            }    
        }
    }
}


void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    bt_init();
    motor_init(&test_motor, TEST_MOTOR_STEP, TEST_MOTOR_DIR, TEST_MOTOR_EN, TEST_MOTOR_CHANNEL);
    cmd_queue = xQueueCreate(10, sizeof(uint64_t)); 

    xTaskCreatePinnedToCore(bt_reciever_parser, "bt_reciever", 4096, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(command_parser, "robot_command_parser", 4096, NULL, 5, NULL, 1);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
