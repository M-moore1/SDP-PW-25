#ifndef STEPPER_MOTOR_H
#define STEPPPER_MOTOR_H

#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_timer.h"

// Configuration constants
#define STEPPER_LEDC_RESOLUTION  LEDC_TIMER_8_BIT
#define STEPPER_LEDC_DUTY        128   // 50% of 255
#define PULSE_DURATION_US        60000 // 60 ms

typedef enum {
    MOTOR_DISABLE = 0,
    MOTOR_IDLE    = 1,
    MOTOR_RUNNING = 2
} motor_state_t;


typedef struct step_mot {
    int step_gpio;
    int dir_gpio;
    int en_gpio; 
    esp_timer_handle_t stop_timer;
    ledc_channel_t channel;
    ledc_timer_t timer_sel;
    motor_state_t status; 
} step_mot_t;

void motor_init(step_mot_t* m, const int step_pin, const int dir_pin, const int en_pin, ledc_channel_t channel);
void stepper_enable(step_mot_t* m);
void stepper_disable(step_mot_t* m);
uint32_t map_speed_to_hz(int speed);
void motor_pulse(step_mot_t *motor, uint32_t speed, int dir);

#endif