#ifndef PINOUT_H
#define PINOUT_H

#include "driver/gpio.h"
#include "driver/ledc.h"



// TEST MOTOR
#define TEST_MOTOR_STEP    GPIO_NUM_32
#define TEST_MOTOR_DIR     GPIO_NUM_33
#define TEST_MOTOR_EN      GPIO_NUM_25
#define TEST_MOTOR_CHANNEL LEDC_CHANNEL_0

//Front Left Motor
#define FL_MOTOR_STEP    GPIO_NUM_3 // RX
#define FL_MOTOR_DIR     GPIO_NUM_19
#define FL_MOTOR_EN      GPIO_NUM_23
#define FL_MOTOR_PWM     LEDC_CHANNEL_0

//Front Right Motor
#define FR_MOTOR_STEP    GPIO_NUM_15
#define FR_MOTOR_DIR     GPIO_NUM_2
#define FR_MOTOR_EN      GPIO_NUM_23
#define FR_MOTOR_PWM     LEDC_CHANNEL_1

//Back Left Motor
#define BL_MOTOR_STEP    GPIO_NUM_25
#define BL_MOTOR_DIR     GPIO_NUM_33
#define BL_MOTOR_EN      GPIO_NUM_23
#define BL_MOTOR_PWM     LEDC_CHANNEL_2

//Back Right Motor
#define BR_MOTOR_STEP    GPIO_NUM_12
#define BR_MOTOR_DIR     GPIO_NUM_14
#define BR_MOTOR_EN      GPIO_NUM_23
#define BR_MOTOR_PWM     LEDC_CHANNEL_3

// Command Types
typedef enum {
    CONTROL_CMD      = 0x01,  
    POSE_CMD         = 0x02,
    System_CMD       = 0x03,  
    Query_CMD        = 0x04,  
    STATUS_CMD       = 0x05,
    HEALTH_CMD       = 0x06,
    ACK_CMD          = 0x07,
    HPR_CMD          = 0x08

} command_type_t;

// Control Command Structure
typedef struct __attribute__((packed)) {
    uint64_t pl      : 2;  // Bits 0-1
    uint64_t type    : 5;  // Bits 2-6
    uint64_t w       : 1;  // Bit 7
    uint64_t a       : 1;  // Bit 8
    uint64_t s       : 1;  // Bit 9
    uint64_t d       : 1;  // Bit 10
    uint64_t speed   : 7;  // Bits 11-17
    uint64_t unused  : 46; // Bits 18-63
} control_format_t;

// Arm Command Structure
typedef struct __attribute__((packed)) {
    uint64_t pl       : 2;  // Bits 0-1       
    uint64_t type     : 5;  // Bits 2-6       
    uint64_t up       : 1;  // Bit 7          
    uint64_t down     : 1;  // Bit 8          
    uint64_t left     : 1;  // Bit 9          
    uint64_t right    : 1;  // Bit 10         
    uint64_t in       : 1;  // Bit 11         
    uint64_t out      : 1;  // Bit 12         
    uint64_t speed    : 7;  // Bits 13-19     
    uint64_t reset    : 1;  // Bit 20         
    uint64_t id       : 11; // Bits 21-31     
    uint64_t unused   : 32; // Bits 32-63    
} arm_format_t;


// System Command Structure
typedef struct __attribute__((packed)) {
    uint64_t pl          : 2;  // Bits 0-1
    uint64_t type        : 5;  // Bits 2-6
    uint64_t instruction : 4;  // Bits 7-10
    uint64_t ac          : 10; // Bits 11-20
    uint64_t id          : 11; // Bits 21-31
    uint64_t specific    : 32; // Bits 32-63
} system_format_t;

// System Instruction
enum system_instructions {
    DISCONNECT        = 0x01,
    Connect_Reconnect = 0x02,
    SECURITY_LEVEL    = 0x03
};

// Query Command Structure
typedef struct __attribute__((packed)) {
    uint64_t pl          : 2;  // Bits 0-1 (Priority Level)
    uint64_t type        : 5;  // Bits 2-6
    uint64_t instruction : 4;  // Bits 7-10
    uint64_t id          : 11; // Bits 11-21
    uint64_t r           : 1;  // Bit 22 (Read/Request Bit)
    uint64_t extra       : 41; // Bits 23-63 (Padding/Extra)
} query_format_t;

// TODO Query instructions enum

// Acknowledge Command Structure
typedef struct __attribute__((packed)) {
    uint64_t pl                  : 2;  // Bits 0-1       
    uint64_t type                : 5;  // Bits 2-6       
    uint64_t id                  : 11; // Bits 7-17      
    uint64_t result_code         : 5;  // Bits 18-22     
    uint64_t instruction_specific: 41; // Bits 23-63     
} ack_format_t;

// Acknowledgment Result Types
enum result_code {
    RESULT_SUCCESS             = 0x00, // Command executed successfully
    RESULT_UNKNOWN_CMD         = 0x01, // Unknown command type
    RESULT_INVALID_PARAMS      = 0x02, // Invalid or conflicting parameters
    RESULT_UNSUPPORTED_CMD     = 0x03, // Command valid but not supported
    RESULT_AUTH_FAIL           = 0x04, // Authentication failed
    RESULT_DUPLICATE_PACKET    = 0x05, // Duplicate or old packet detected
    RESULT_BT_NOT_INITIALIZED  = 0x06, // Bluetooth connection not initialized
    RESULT_MOTORS_DISABLED     = 0x07  // Motors currently disabled
    
};

typedef struct __attribute__((packed)) {
    uint64_t pl         : 2;  // Bits 0-1       Priority Level
    uint64_t type       : 5;  // Bits 2-6       Packet Type (HPR)
    uint64_t alert_type : 5;  // Bits 7-11      Alert Type
    uint64_t extra      : 52; // Bits 12-63     Reserved / extra bits
} hpr_format_t;

typedef union {
    uint8_t bytes[8];      // For raw BT transmission
    uint64_t raw;          // For logging/debugging
    control_format_t ctrl; // Map to Control Commands
    system_format_t sys;   // Map to System Commands
    query_format_t query;  // Map to Query Commands
    arm_format_t arm;      // Map to Arm Commands
    ack_format_t ack;      // Map to Acknowledgment Commands
    hpr_format_t hpr;
} robot_bt_packet_t;


#endif