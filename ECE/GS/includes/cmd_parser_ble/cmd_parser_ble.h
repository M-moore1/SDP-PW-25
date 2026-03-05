#ifndef CMD_PARSER_BLE_H
#define CMD_PARSER_BLE_H

/*
 * cmd_parser_ble.h
 * -----------------------------------------------------------------------------
 * BLE version of the command parser for gs_bridgeble.c.
 *
 * This file is meant to replace includes/cmd_parser/cmd_parser.{h,c} when you
 * build the BLE bridge target, so you DON'T pull in any rn42/bt2 symbols.
 *
 * It keeps the same entrypoint name used by gs_bridge*.c:
 *   void handle_node_json(int uart_fd, int uds_fd, const char *json_str);
 *
 * so you can swap the include and source file in your Makefile without changing
 * the bridge logic.
 *
 * JSON command schema (supported here):
 *   1) CONNECT:
 *      {"type":"CONNECT","mac":"441d64f17066"}   // mac optional if bridge auto-connects
 *
 *   2) DISCONNECT:
 *      {"type":"DISCONNECT"}
 *
 *   3) LINK CHECK:
 *      {"type":"GK"} or {"type":"LINK"}          // returns {"type":"LINK","connected":0/1}
 *
 *   4) PLAINTEXT INSTRUCTION (u64):
 *      {"type":"INSTR","instruction":1234}
 *      {"type":"INSTR","instruction":"0x0123ABCD..."}  // hex string OK
 *
 *   5) ENCRYPTED WORD (u64) - AES-GCM in bridge, same as BT2 version:
 *      {"type":"ENC_WORD","word":1234}
 *      {"type":"ENC_WORD","word":"0x..."}
 *
 * If your project uses a different schema (e.g., nested objects), adapt in
 * cmd_parser_ble.c in one place: parse_u64_from_json().
 * -----------------------------------------------------------------------------
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void handle_node_json(int uart_fd, int uds_fd, const char *json_str);

#ifdef __cplusplus
}
#endif

#endif
