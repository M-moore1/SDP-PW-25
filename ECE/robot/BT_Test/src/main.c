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

step_mot_t front_left;
step_mot_t front_right;
step_mot_t back_left;
step_mot_t back_right;
static QueueHandle_t cmd_queue = NULL;


void bt_reciever_parser(void *pvParameters)
{
    uint8_t received_packet[156]; 

    while (1) {
        if (xQueueReceive(bt_recieve_queue, received_packet, portMAX_DELAY)) {
            const char *acknowledge = "Bytes Recieved\n";
            esp_spp_write(spp_handle, strlen(acknowledge), (uint8_t *)acknowledge);
            printf("Received 156 bytes: \n");

            for (int i = 0; i < 156; i++) {
                printf("%02X ", received_packet[i]);
                if ((i + 1) % 16 == 0) printf("\n"); // New line every 16 bytes for readability
            }
            printf("\n");
            printf("End of Bytes");

            if (security_flag) {
                printf("Decrypting bits...\n");
                
            }

            if (xQueueSend(cmd_queue, received_packet, 0) != pdPASS) {
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
                    control_cmd(cmd.ctrl, &front_left, &front_right, &back_left, &back_right);

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
    motor_init(&front_left,  FL_MOTOR_STEP, FL_MOTOR_DIR, FL_MOTOR_EN, FL_MOTOR_PWM);
    motor_init(&front_right, FR_MOTOR_STEP, FR_MOTOR_DIR, FR_MOTOR_EN, FR_MOTOR_PWM);

    motor_init(&back_left,   BL_MOTOR_STEP, BL_MOTOR_DIR, BL_MOTOR_EN, BL_MOTOR_PWM);
    motor_init(&back_right,  BR_MOTOR_STEP, BR_MOTOR_DIR, BR_MOTOR_EN, BR_MOTOR_PWM);
    cmd_queue = xQueueCreate(10, sizeof(uint64_t)); 

    xTaskCreatePinnedToCore(bt_reciever_parser, "bt_reciever", 4096, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(command_parser, "robot_command_parser", 4096, NULL, 5, NULL, 1);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
