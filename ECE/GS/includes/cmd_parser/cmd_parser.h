#ifndef CMD_PARSER_H
#define CMD_PARSER_H

#include "cJSON.h" // CHANGE

#include "../cmd_structure.h"
#include "../includes/ble/pmod_esp32.h"
#include "../hardware_crypto/software_cryptography.h"
//#include "../json_uds/json_uds.h"

extern volatile int security_level;
extern volatile int connection_status;
extern volatile int authorization_code;

void sys_cmd(int uart_fd, system_format_t sys_inst);
void query_cmd(int uart_fd, query_format_t query_inst);
int handle_encrypted_data(int uart_fd, int uds_fd, const char *encrypt_str);
int handle_node_json(int uart_fd, int uds_fd, const char *json_str);

#endif
