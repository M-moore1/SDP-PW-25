#ifndef PINOUT_H
#define PINOUT_H

#include "driver/gpio.h"
#include "driver/ledc.h"

// TEST MOTOR
#define TEST_MOTOR_STEP    GPIO_NUM_32
#define TEST_MOTOR_DIR     GPIO_NUM_33
#define TEST_MOTOR_EN      GPIO_NUM_25
#define TEST_MOTOR_CHANNEL LEDC_CHANNEL_0

typedef enum {
    CONTROL_CMD      = 0x01,  
    POSE_CMD         = 0x02,
    System_CMD       = 0x03,  
    Query_CMD        = 0x04,  
    STATUS_CMD       = 0x05,
    HEALTH_CMD       = 0x06,
    ACK_CMD          = 0x07,
    HPR_CMD          = 0x08

} robot_cmd_type_t;


#endif