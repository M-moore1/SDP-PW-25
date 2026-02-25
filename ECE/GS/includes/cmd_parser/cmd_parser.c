#include "cmd_parser.h"

volatile int security_level = 0;
volatile int connection_status = 0;
volatile int authorization_code = 0x3FF;

// Do sys instructions for the robot
void sys_cmd(int uart_fd, system_format_t sys_inst){
  switch(sys_inst.instruction){
    case SECURITY_LEVEL:
      sys_inst.specific = security_level;

      break;
    case Connect_Reconnect:
      rn42_connect_mac(uart_fd, "004B1224B0A6");

      break;

    case DISCONNECT:
      rn42_disconnect(uart_fd);
      break;
  }
  
}

void query_cmd(int uart_fd, query_format_t query_inst){

}

// ------------------------- Handle Node JSON -------------------------
// Parse incoming JSON from Node and transmit appropriate 64-bit word(s) over UART.
int handle_node_json(int uart_fd, int uds_fd, const char *json_str) {
  cJSON *root = cJSON_Parse(json_str);
  if (!root) {
    uds_send_json(uds_fd, "{\"type\":\"ERR\",\"msg\":\"bad json\"}");
    return -1;
  }

  // Placeholder for future AES-GCM decryption
  if (security_level) {
    // TODO: decrypt json_str if your team moves to encrypted JSON
    

  }

  cJSON *type_item = cJSON_GetObjectItemCaseSensitive(root, "type");
  if (!cJSON_IsString(type_item) || !type_item->valuestring) {
    cJSON_Delete(root);
    uds_send_json(uds_fd, "{\"type\":\"ERR\",\"msg\":\"missing type\"}");
    return -1;
  }

  const char *t = type_item->valuestring;
  robot_bt_packet_t packet = {0}; // Initialize to clear all 64 bits (including "unused")
  // TODO: Add Arm instructions
  // CONTROL (C)
  if (strcmp(t, "C") == 0) {
    uint8_t f, b, l, r_move, speed, pl;

    if (json_get_u8(root, "forward",        &f,     0, 1)   ||
        json_get_u8(root, "backward",       &b,     0, 1)   ||
        json_get_u8(root, "left",           &l,     0, 1)   ||
        json_get_u8(root, "right",          &r_move,0, 1)   ||
        json_get_u8(root, "speed",          &speed, 0, 100) ||
        json_get_u8(root, "priority_level", &pl,    0, 3)) 
    {
      uds_send_json(uds_fd, "{\"type\":\"ERR\",\"msg\":\"bad C fields\"}");
      cJSON_Delete(root);
      return -1;
    }

    packet.ctrl.type  = CONTROL_CMD;
    packet.ctrl.pl    = pl;
    packet.ctrl.w     = f;
    packet.ctrl.s     = b;
    packet.ctrl.a     = l;
    packet.ctrl.d     = r_move;
    packet.ctrl.speed = speed;
  }

  // SYSTEM (S)
  else if (strcmp(t, "S") == 0) {
    uint8_t  pl, instr;
    uint16_t id, ac;
    uint32_t spec;

    if (json_get_u8(root, "instruction",         &instr, 0, 15)   ||
        json_get_u16(root,"Authorization_Code",  &ac,    0, 1023) ||
        json_get_u8(root, "priority_level",      &pl,    0, 3)    ||
        json_get_u16(root,"id",                  &id,    0, 2047) ||
        json_get_u32(root,"instruction_specific",&spec)) // Assuming 3-arg version
    {
      uds_send_json(uds_fd, "{\"type\":\"ERR\",\"msg\":\"bad S fields\"}");
      cJSON_Delete(root);
      return -1;
    }

    packet.sys.type         = System_CMD;
    packet.sys.pl           = pl;
    packet.sys.instruction  = instr;
    packet.sys.ac           = ac;
    packet.sys.id           = id;
    packet.sys.specific     = spec;

    sys_cmd(uart_fd, packet.sys);
  }

  // QUERY (Q)
  else if (strcmp(t, "Q") == 0) {
    uint8_t  pl, instr, report;
    uint16_t id;

    if (json_get_u8(root, "instruction",    &instr,  0, 15)  ||
        json_get_u8(root, "report",         &report, 0, 1)   ||
        json_get_u8(root, "priority_level", &pl,     0, 3)   ||
        json_get_u16(root,"id",             &id,     0, 2047)) 
    {
      uds_send_json(uds_fd, "{\"type\":\"ERR\",\"msg\":\"bad Q fields\"}");
      cJSON_Delete(root);
      return -1;
    }

    packet.query.type        = Query_CMD;
    packet.query.pl          = pl;
    packet.query.instruction = instr;
    packet.query.id          = id;
    packet.query.r           = report; 

      
    query_cmd(uart_fd, packet.query);
  }
    
  // ERROR HANDLING
  else {
      uds_send_json(uds_fd, "{\"type\":\"ERR\",\"msg\":\"unknown type\"}");
      cJSON_Delete(root);
      return -1;
  }

  // Cleanup JSON memory
  cJSON_Delete(root);

  if (security_level) {
    // TODO: Wrap packet.raw in AES-GCM 33-byte envelope here
    // return uart_send_encrypted(uart_fd, packet.raw);
  }
  // TODO add priority Queue   
  return uart_send_instruction(uart_fd, packet.raw);
    

  return 0;
}