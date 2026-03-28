#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "stepper_motor.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "driver/uart.h"

#include "nvs_flash.h"
#include "esp_log.h"
#include "Robot_BLE.h"
#include "pinout.h"
#include "robot_commands.h"
#include "i2c.h"
#include "imu.h"

step_mot_t test_motor;
step_mot_t front_left;
step_mot_t front_right;
step_mot_t back_left;
step_mot_t back_right;

static QueueHandle_t cmd_queue = NULL;

void ble_recieve_parser(void *pvParameters)
{
uint8_t received_packet[packet_size];


while (1) {

    if (xQueueReceive(ble_recieve_queue, &received_packet, portMAX_DELAY))
    {
        printf("Received %d bytes:\n", packet_size);

        for (int i = 0; i < packet_size; i++) {
            printf("%02X ", received_packet[i]);
            if ((i + 1) % 16 == 0) printf("\n");
        }
        printf("\nEnd of Bytes\n");

        // ===============================
        // ✅ EXTRACT FIRST 8 BYTES
        // ===============================
        uint64_t inst;
        memcpy(&inst, received_packet, 8);

        if (xQueueSend(cmd_queue, &inst, 0) != pdPASS) {
            ESP_LOGW(GATTS_TABLE_TAG, "cmd_queue full");
        }

        // ===============================
        // ✅ SEND BACK FRAMED RESPONSE
        // ===============================
        uint8_t frame[158];

        frame[0] = 0x0A; // START
        memcpy(&frame[1], received_packet, 156);
        frame[157] = 0x0D; // STOP

        send_bytes(frame, 158);
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
        printf("Command Parse Initiated\n");

        cmd.raw = inst;

        command_type_t cmd_type = (command_type_t)cmd.ctrl.type;
        ESP_LOGI("CMD", "Extracted type: %u", cmd_type);

        switch (cmd_type){

            case CONTROL_CMD:
                printf("I got a control instruction\n");
                control_cmd(cmd.ctrl, &front_left, &front_right, &back_left, &back_right);
            break;

            case ARM_CMD:
                printf("I got a Pose instruction\n");
                arm_cmd(inst, &test_motor, &test_motor, &test_motor, &test_motor);
            break;

            case System_CMD:
                printf("I got a System instruction\n");
                system_cmd(cmd.sys, &test_motor, &test_motor, &test_motor, &test_motor);
            break;
            
            case Query_CMD:
                printf("I got a Query Instruction\n");
                query_cmd(cmd.query, &test_motor, &test_motor, &test_motor, &test_motor);
            break;
            
            default:
                printf("Invalid Command\n");
            break;
        }
    }
}


}

void app_main()
{
ESP_ERROR_CHECK(nvs_flash_init());
robot_ble_init();


motor_init(&front_left,  FL_MOTOR_STEP, FL_MOTOR_DIR, FL_MOTOR_EN, FL_MOTOR_PWM);
motor_init(&front_right, FR_MOTOR_STEP, FR_MOTOR_DIR, FR_MOTOR_EN, FR_MOTOR_PWM);
motor_init(&back_left,   BL_MOTOR_STEP, BL_MOTOR_DIR, BL_MOTOR_EN, BL_MOTOR_PWM);
motor_init(&back_right,  BR_MOTOR_STEP, BR_MOTOR_DIR, BR_MOTOR_EN, BR_MOTOR_PWM);

// ✅ FIXED: queue holds ONLY 8-byte commands
cmd_queue = xQueueCreate(10, sizeof(uint64_t));

xTaskCreatePinnedToCore(ble_recieve_parser, "ble_recieve_parser", 4096, NULL, 5, NULL, 1);
xTaskCreatePinnedToCore(command_parser, "robot_command_parser", 4096, NULL, 5, NULL, 1);

while (1) {
    vTaskDelay(pdMS_TO_TICKS(2000));
}


}
