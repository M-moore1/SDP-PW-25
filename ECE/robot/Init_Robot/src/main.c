#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "stepper_motor.h"
#include "robot_bt.h"
#include "pinout.h"
#include "robot_commands.h"




// Create the motor instance
step_mot_t front_left;
step_mot_t front_right;
step_mot_t back_left;
step_mot_t back_right;

void app_main(void) {

    motor_init(&front_left,  FL_MOTOR_STEP, FL_MOTOR_DIR, FL_MOTOR_EN, FL_MOTOR_PWM);
    motor_init(&front_right, FR_MOTOR_STEP, FR_MOTOR_DIR, FR_MOTOR_EN, FR_MOTOR_PWM);

    motor_init(&back_left,   BL_MOTOR_STEP, BL_MOTOR_DIR, BL_MOTOR_EN, BL_MOTOR_PWM);
    motor_init(&back_right,  BR_MOTOR_STEP, BR_MOTOR_DIR, BR_MOTOR_EN, BR_MOTOR_PWM);

    while(1) {
        for (int i = 0; i < 50; i++) {
            motor_pulse(&back_left, 100, 1);
            motor_pulse(&back_right, 100, 1);
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        
         for (int i = 0; i < 50; i++) {
            motor_pulse(&front_left, 100, 1);
            

            motor_pulse(&front_right, 100, 1);
            vTaskDelay(pdMS_TO_TICKS(50));
        }
        

        // ---- 1 second delay ----
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}