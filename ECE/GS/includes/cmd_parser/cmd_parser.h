#ifndef CMD_PARSER_H
#define CMD_PARSER_H


#include "../cmd_structure.h"
#include "../bt2/bt2.h"
#include "cJSON.h"

extern volatile int security_level;
extern volatile int connection_status;
extern volatile int authorization_code;


void query_cmd(int uart_fd, query_format_t query_inst);
int handle_node_json(int uart_fd, int uds_fd, const char *json_str);

#endif
