#ifndef CMD_STRUCTURE_H
#define CMD_STRUCTURE_H

#include <stdint.h>

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

typedef struct __attribute__((packed)) {
    uint64_t pl          : 2;  // Bits 0-1
    uint64_t type        : 5;  // Bits 2-6
    uint64_t instruction : 4;  // Bits 7-10
    uint64_t ac          : 10; // Bits 11-20
    uint64_t id          : 11; // Bits 21-31
    uint64_t specific    : 32; // Bits 32-63
} system_format_t;

typedef enum {
    DISCONNECT        = 0X01,
    Connect_Reconnect = 0x02,
    SECURITY_LEVEL    = 0x03
} system_instructions_t;


typedef struct __attribute__((packed)) {
    uint64_t pl          : 2;  // Bits 0-1 (Priority Level)
    uint64_t type        : 5;  // Bits 2-6
    uint64_t instruction : 4;  // Bits 7-10
    uint64_t id          : 11; // Bits 11-21
    uint64_t r           : 1;  // Bit 22 (Read/Request Bit)
    uint64_t extra       : 41; // Bits 23-63 (Padding/Extra)
} query_format_t;


typedef union {
    uint8_t bytes[8];      // For raw BT transmission
    uint64_t raw;          // For logging/debugging
    control_format_t ctrl; // Map to Control layout
    system_format_t sys;   // Map to System layout
    query_format_t query;  // Map to Query layout
} robot_bt_packet_t;

#endif