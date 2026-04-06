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
      ble_connect(uart_fd, ESP32_MAC);

      break;

    case DISCONNECT:
      ble_discon(uart_fd);
      break;
  }
  
}

void query_cmd(int uart_fd, query_format_t query_inst){

}

int handle_encrypted_data(int uart_fd, int uds_fd, const char *encrypt_str){
  // TEST WITH NO GS CRYPTOGRAPHY JUST SEND to ROBOT
  //uart_send_str(uart_fd, encrypt_str, str_len(encrypt_str));
  // END
  // Implement DECRYPTION then send to handle json also add json send back feedback

  //Do Decrypt
  uint8_t decrypted_raw[CT_SZ + 1] = {0};
  int len = sw_decryption(encrypt_str, decrypted_raw);

  if (len < 0) { return - 1; }

  int json_len = strip_pad(decrypted_raw, len);
  decrypted_raw[json_len] = '\0';

  printf("Decrypted JSON (%d bytes): %s\n", json_len, (const char *)decrypted_raw);
  handle_node_json(uart_fd, uds_fd, (const char *)decrypted_raw);


  return 0;
}

// ------------------------- Handle Node JSON -------------------------
// Parse incoming JSON from Node and transmit appropriate 64-bit word(s) over UART.
int handle_node_json(int uart_fd, int uds_fd, const char *json_str) {
  cJSON *root = cJSON_Parse(json_str);
  if (!root) {
    uds_send_json(uds_fd, "{\"type\":\"ERR\",\"msg\":\"bad json\"}");
    return -1;
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
        json_get_u16(root,"Authorization_Code",  &ac,    0, 1024) ||
        json_get_u8(root, "priority_level",      &pl,    0, 3)    ||
        json_get_u16(root,"id",                  &id,    0, 2047) ||
        json_get_u32(root,"instruction_specific",&spec)) // Assuming 3-arg version
    {
      printf("I GOT AN ERROR WITH THE S INSTRUCTION\n");
      uds_send_json(uds_fd, "{\"type\":\"ERR\",\"msg\":\"bad S fields\"}");
      cJSON_Delete(root);
      return -1;
    }
    printf("I GOT AN S INSTRUCTION\n");

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
    char encrypted_string[PAYLOAD_HEX_STR_LEN + 1] = {0};
    if (encrypt_instr(packet.bytes, encrypted_string) != 0) {
      printf("ERROR: encryption failed\n");
      return 1;
    }

    uint8_t payload[PAYLOAD_BYTES];
    for (int i = 0; i < PAYLOAD_BYTES; i++) {
      char byte_str[3] = { encrypted_string[i * 2], encrypted_string[i * 2 + 1], '\0' };
      payload[i] = (uint8_t)strtol(byte_str, NULL, 16);
    }
    
    return ble_send_pkt(uart_fd, payload, PAYLOAD_BYTES);
    
  }
  // TODO add priority Queue
  // STEPHEN DO above the security level if else about cause secure packets would not be tracked   
  
  uint16_t id_to_track = 0;

  if (strcmp(t, "C") == 0) {
      id_to_track = 0; // control has no ID
  }
  else if (strcmp(t, "S") == 0) {
      id_to_track = packet.sys.id;
  }
  else if (strcmp(t, "Q") == 0) {
      id_to_track = packet.query.id;
  }

  // Only store if valid
  if (id_to_track != 0) {
      store_command(id_to_track);
  }

  return ble_send_instruction(uart_fd, packet.bytes);
}

cJSON* robot_packet_to_json(robot_bt_packet_t pkt) {

    cJSON *root = cJSON_CreateObject();
    uint8_t type = pkt.ctrl.type;

    switch(type) {

        // =========================
        // ROBOT UPDATE (SR)
        // =========================
        case ROBOT_UPDATE_CMD:

            cJSON_AddStringToObject(root, "type", "SR");

            if (pkt.nav.part == 0) {
                cJSON_AddNumberToObject(root, "px", pkt.nav.pos_x);
                cJSON_AddNumberToObject(root, "py", pkt.nav.pos_y);
                cJSON_AddNumberToObject(root, "pz", pkt.nav.pos_z);
                cJSON_AddNumberToObject(root, "speed", pkt.nav.speed);
            }

            else if (pkt.pose.part == 1) {
                cJSON_AddNumberToObject(root, "yaw", pkt.pose.yaw);
                cJSON_AddNumberToObject(root, "pitch", pkt.pose.pitch);
                cJSON_AddNumberToObject(root, "roll", pkt.pose.roll);
            }

            else if (pkt.inert.part == 2) {
                cJSON_AddNumberToObject(root, "ax", pkt.inert.accel_x);
                cJSON_AddNumberToObject(root, "ay", pkt.inert.accel_y);
                cJSON_AddNumberToObject(root, "az", pkt.inert.accel_z);

                cJSON_AddNumberToObject(root, "gx", pkt.inert.gyro_x);
                cJSON_AddNumberToObject(root, "gy", pkt.inert.gyro_y);
                cJSON_AddNumberToObject(root, "gz", pkt.inert.gyro_z);
            }

            break;

        // =========================
        // HEALTH
        // =========================
        case HEALTH_CMD:

            cJSON_AddStringToObject(root, "type", "HR");

            cJSON_AddNumberToObject(root, "battery", pkt.health.battery);
            cJSON_AddNumberToObject(root, "security", pkt.health.sec_lvl);
            cJSON_AddNumberToObject(root, "motor_enabled", pkt.health.motor_en);

            break;

        // =========================
        // ACK
        // =========================
        case ACK_CMD:

            cJSON_AddStringToObject(root, "type", "ACK");

            cJSON_AddNumberToObject(root, "id", pkt.ack.id);
            cJSON_AddNumberToObject(root, "result", pkt.ack.result_code);

            break;

        // =========================
        // HPR
        // =========================
        case HPR_CMD:

            cJSON_AddStringToObject(root, "type", "HPR");

            cJSON_AddNumberToObject(root, "alert", pkt.hpr.alert_type);

            break;

        default:
            cJSON_AddStringToObject(root, "type", "UNKNOWN");
    }

    return root;
}