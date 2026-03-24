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

// Packet Info
#define packet_size  156

// Command Types
typedef enum {
    CONTROL_CMD      = 0x01,  
    ARM_CMD         = 0x02,
    System_CMD       = 0x03,  
    Query_CMD        = 0x04,  
    ROBOT_UPDATE_CMD = 0x05,  
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


// Part 0: Navigation (Position & Speed)
typedef struct __attribute__((packed)) {
    uint64_t pl      : 2;  // Bits 0-1
    uint64_t type    : 5;  // Bits 2-6 (0x05)
    uint64_t part    : 2;  // Bits 7-8 (00)
    uint64_t speed   : 7;  // Bits 9-15
    int64_t  pos_x   : 16; // Bits 16-31 (Signed mm)
    int64_t  pos_y   : 16; // Bits 32-47 (Signed mm)
    int64_t  pos_z   : 16; // Bits 48-63 (Signed mm)
} nav_format_t;

// Part 1: Pose (Euler Angles / Orientation)
typedef struct __attribute__((packed)) {
    uint64_t pl      : 2;  // Bits 0-1
    uint64_t type    : 5;  // Bits 2-6 (0x05)
    uint64_t part    : 2;  // Bits 7-8 (01)
    uint64_t yaw     : 18; // Bits 9-26 (Unsigned 0.001 precision)
    int64_t  pitch   : 18; // Bits 27-44 (Signed 0.001 precision)
    int64_t  roll    : 18; // Bits 45-62 (Signed 0.001 precision)
    uint64_t reserved: 1;  // Bit 63
} pose_format_t;

// Part 2: Inertia (Accel & Gyro)
typedef struct __attribute__((packed)) {
    uint64_t pl      : 2;  // Bits 0-1
    uint64_t type    : 5;  // Bits 2-6 (0x05)
    uint64_t part    : 2;  // Bits 7-8 (10)
    int64_t  accel_x : 9;  // Bits 9-17 (Signed 0.1 precision)
    int64_t  accel_y : 9;  // Bits 18-26
    int64_t  accel_z : 9;  // Bits 27-35
    int64_t  gyro_x  : 9;  // Bits 36-44 (Signed 0.1 precision)
    int64_t  gyro_y  : 9;  // Bits 45-53
    int64_t  gyro_z  : 9;  // Bits 54-62
    uint64_t reserved: 1;  // Bit 63
} inertia_format_t;

// Health Status (HR)
typedef struct __attribute__((packed)) {
    uint64_t pl      : 2;  // Bits 0-1
    uint64_t type    : 5;  // Bits 2-6 (0x06)
    uint64_t part    : 2;  // Bits 7-8 (11)
    uint64_t battery : 7;  // Bits 9-15 (%)
    uint64_t sec_lvl : 2;  // Bits 16-17
    uint64_t motor_en: 1;  // Bit 18 (0=Off, 1=On)
    uint64_t unused  : 45; // Bits 19-63
} health_format_t;

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
    nav_format_t nav;      // Navigation (Part 0)
    pose_format_t pose;    // Pose/Orientation (Part 1)
    inertia_format_t inert;// Inertia (Part 2)
    health_format_t health;// Health (Part 11)
} robot_bt_packet_t;


#endif