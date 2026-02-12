#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "stepper_motor.h"


#define TEST_MOTOR_STEP GPIO_NUM_32
#define TEST_MOTOR_DIR  GPIO_NUM_33
#define TEST_MOTOR_EN   GPIO_NUM_25
#define TEST_MOTOR_CHANNEL LEDC_CHANNEL_0

// Create the motor instance
step_mot_t test_motor;
step_mot_t test_motor_2;

void app_main(void) {

    motor_init(&test_motor, TEST_MOTOR_STEP, TEST_MOTOR_DIR, TEST_MOTOR_EN, TEST_MOTOR_CHANNEL);
    motor_init(&test_motor_2, 18, TEST_MOTOR_DIR, TEST_MOTOR_EN, TEST_MOTOR_CHANNEL);

    while(1) {
        for (int i = 0; i < 50; i++) {
            motor_pulse(&test_motor, 100, 1);
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        for (int i = 0; i < 50; i++) {
            motor_pulse(&test_motor, 100, 0);
            vTaskDelay(pdMS_TO_TICKS(50));
        }

        // ---- 1 second delay ----
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}