#include "cmd_parser.h"
#include "../cmd_structure.h"

volatile int security_level = 0;
volatile int connection_status = 0;
volatile int authorization_code = 0x3FF;

// Do sys instructions for the robot
int sys_cmd(int uart_fd, system_format_t sys_inst){
  int robot_send_need = 1;

  switch(sys_inst.instruction){
    case SECURITY_LEVEL:
      printf("Changing Security Level\r\n");
      if(sys_inst.specific == 1){
        robot_bt_packet_t packet = {0};
        packet.sys.pl          = sys_inst.pl;
        packet.sys.type        = System_CMD;
        packet.sys.instruction = sys_inst.instruction;
        packet.sys.ac          = sys_inst.ac;
        packet.sys.id          = sys_inst.id;
        packet.sys.specific    = sys_inst.specific;
        ble_send_instruction(uart_fd, packet.bytes);
        robot_send_need = 0;
      }
      security_level = sys_inst.specific;
    break;

    case Connect_Reconnect:
      printf("Attempting Connection\r\n");
      if (ble_connect(uart_fd, NULL) < 0){ connection_status = 0;}
      else { connection_status = 1; }
      robot_send_need = 0;

      // ADD SEND ACK to UI FUNCTION
      break;

    case DISCONNECT:
      if (ble_discon(uart_fd) == 0) connection_status = 0;
      robot_send_need = 0;
      printf("Disconnected\r\n");
      // ADD SEND ACK to UI FUNCTION

      break;

    case ROBOT_NAME_CHANGE:
      uint32_t specific_val = sys_inst.specific;
      char new_name[5] = {0};
      memcpy(new_name, &specific_val, 4);
      new_name[4] = '\0';
      pmod_name(uart_fd, new_name, NULL); 
      printf("Name Changed\r\n");
    break;

    case GS_BLE_RESET:

      robot_send_need = 0;
      if (pmod_esp32_reset(uart_fd) == 0) {
        send_at_cmd(uart_fd, "ATE0\r\n", NULL, NULL, 50);
        connection_status = 0;
      }
      // SEND ACK back to the UI
    break;
  }
  
  return robot_send_need;
}

int query_cmd(int uart_fd, query_format_t query_inst){
  int robot_send_need = 1;

  switch(query_inst.instruction){
    case CONNNECTION_STAT:
      robot_send_need = 0;

      // SEND ACK to UI
    break;

    case SECURITY_STATUS:
      robot_send_need = 0;

      // Send ACK to UI
    break;

    case ROBOT_NAME:
      robot_send_need = 0;

      // Send ACK to UI
    break;

  }
  return robot_send_need;
}

int handle_encrypted_data(int uart_fd, int uds_fd, const char *encrypt_str) {
    if (!encrypt_str) {
        fprintf(stderr, "[encrypt] ERROR: null input\n");
        return -1;
    }

    /* Strip all whitespace into a clean buffer */
    char clean[PAYLOAD_HEX_STR_LEN + 1] = {0};
    size_t clean_len = 0;
    for (size_t i = 0; encrypt_str[i] != '\0'; i++) {
        unsigned char c = (unsigned char)encrypt_str[i];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') continue;
        if (clean_len >= PAYLOAD_HEX_STR_LEN) {
            fprintf(stderr, "[encrypt] ERROR: input too long after stripping whitespace\n");
            return -2;
        }
        clean[clean_len++] = encrypt_str[i];
    }
    clean[clean_len] = '\0';

    if (clean_len != PAYLOAD_HEX_STR_LEN) {
        fprintf(stderr, "[encrypt] ERROR: bad length — got %zu, expected %d\n",
                clean_len, PAYLOAD_HEX_STR_LEN);
        return -2;
    }

    uint8_t encrypted_bytes[TOTAL_SZ] = {0};
    for (int i = 0; i < TOTAL_SZ; i++) {
        if (sscanf(clean + i * 2, "%02hhx", &encrypted_bytes[i]) != 1) {
            fprintf(stderr, "[encrypt] ERROR: hex parse failed at byte %d\n", i);
            return -3;
        }
    }

    char json_out[CT_SZ + 1] = {0};
    int len = decrypt_json(encrypted_bytes, json_out, sizeof(json_out));
    if (len < 0) {
        fprintf(stderr, "[encrypt] ERROR: decrypt_json failed (returned %d)\n", len);
        return -4;
    }

    json_out[CT_SZ] = '\0';

    printf("[encrypt] Decrypted JSON (%d bytes): %s\n", len, json_out);
    fflush(stdout);

    handle_node_json(uart_fd, uds_fd, json_out);
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

  cJSON *type_item = cJSON_GetObjectItemCaseSensitive(root, "T");
  if (!cJSON_IsString(type_item) || !type_item->valuestring) {
    cJSON_Delete(root);
    uds_send_json(uds_fd, "{\"type\":\"ERR\",\"msg\":\"missing type\"}");
    return -1;
  }

  const char *t = type_item->valuestring;
  robot_bt_packet_t packet = {0}; // Initialize to clear all 64 bits (including "unused")
  int send_to_robot = 1;

  //printf("IM PARSING\r\n");
  // CONTROL (C)
  if (strcmp(t, "C") == 0) {
    uint8_t f, b, l, r_move, speed, pl;
    uint16_t id_tag = 0;

    if (json_get_u8(root, "F",  &f,     0, 1)   ||
        json_get_u8(root, "B",  &b,     0, 1)   ||
        json_get_u8(root, "L",  &l,     0, 1)   ||
        json_get_u8(root, "R",  &r_move,0, 1)   ||
        json_get_u8(root, "S",  &speed, 0, 100) ||
        json_get_u8(root, "PL", &pl,    0, 3) )  //||
        //json_get_u8(root, "ID", &pl,    1, 2047)) 
    {
      uds_send_json(uds_fd, "{\"type\":\"ERR\",\"msg\":\"bad C fields\"}");
      cJSON_Delete(root);
      return -1;
    }
    printf("Doing this\r\n");
    packet.ctrl.type  = CONTROL_CMD;
    packet.ctrl.pl    = pl;
    packet.ctrl.w     = f;
    packet.ctrl.s     = b;
    packet.ctrl.a     = l;
    packet.ctrl.d     = r_move;
    packet.ctrl.speed = speed;
    //packet.ctrl.id    = id_tag;
  }

  // ARM (A)
  else if (strcmp(t, "A") == 0) {
    uint8_t  up, down, left, right, in, out, reset, pl;
    uint8_t  speed;
    uint16_t id_tag;

    if (json_get_u8(root,  "U",  &up,    0, 1)    ||
        json_get_u8(root,  "D",  &down,  0, 1)    ||
        json_get_u8(root,  "L",  &left,  0, 1)    ||
        json_get_u8(root,  "R",  &right, 0, 1)    ||
        json_get_u8(root,  "In", &in,    0, 1)    ||
        json_get_u8(root,  "O",  &out,   0, 1)    ||
        json_get_u8(root,  "S",  &speed, 0, 100)  ||
        json_get_u8(root,  "Re", &reset, 0, 1)    ||
        json_get_u8(root,  "PL", &pl,    0, 3)    ||
        json_get_u16(root, "ID", &id_tag, 1, 2047))
    {
        uds_send_json(uds_fd, "{\"type\":\"ERR\",\"msg\":\"bad A fields\"}");
        cJSON_Delete(root);
        return -1;
    }

    packet.arm.type  = ARM_CMD;
    packet.arm.pl    = pl;
    packet.arm.up    = up;
    packet.arm.down  = down;
    packet.arm.left  = left;
    packet.arm.right = right;
    packet.arm.in    = in;
    packet.arm.out   = out;
    packet.arm.speed = speed;
    packet.arm.reset = reset;
    packet.arm.id    = id_tag;
  }

  // SYSTEM (S)
  else if (strcmp(t, "S") == 0) {
    uint8_t  pl, instr;
    uint16_t id, ac;
    uint32_t spec;
    

    if (json_get_u8(root, "instruction",   &instr, 0, 15)   ||
        json_get_u16(root,"Authorization_Code",  &ac,    0, 1024) ||
        json_get_u8(root, "PL",  &pl,    0, 3)    ||
        json_get_u16(root,"ID",  &id,    0, 2047) ||
        json_get_u32(root,"instruction_specific", &spec)) 
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

    send_to_robot = sys_cmd(uart_fd, packet.sys);
  }

  // QUERY (Q)
  else if (strcmp(t, "Q") == 0) {
    uint8_t  pl, instr, report;
    uint16_t id;

    if (json_get_u8(root, "RI",    &instr,  0, 15)  ||
        json_get_u8(root, "R",         &report, 0, 1)   ||
        json_get_u8(root, "PL", &pl,     0, 3)   ||
        json_get_u16(root,"ID",             &id,     0, 2047)) 
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

      
    send_to_robot = query_cmd(uart_fd, packet.query);
  }
    
  // ERROR HANDLING
  else {
      uds_send_json(uds_fd, "{\"type\":\"ERR\",\"msg\":\"unknown type\"}");
      cJSON_Delete(root);
      return -1;
  }

  // Cleanup JSON memory
  cJSON_Delete(root);

  //Put a connection check and send back ACK
  if (send_to_robot == 1){
    if (security_level == 1) {
      uint8_t ciphertext[TOTAL_SZ] = {0};
      size_t out_len = 0;

      printf("Packet (8 bytes): ");
      for (int i = 0; i < 8; i++) printf("%02X ", packet.bytes[i]);
      printf("\n");

      if (encrypt_cmd(&packet, ciphertext, &out_len) != 0) return -1;
      //printf("Ciphertext (%zu bytes): ", out_len);
      //for (size_t i = 0; i < out_len; i++) printf("%02X ", ciphertext[i]);
      //printf("\n");

      return ble_send_pkt(uart_fd, ciphertext, out_len);
    }
    // TODO add priority Queue   
    return ble_send_instruction(uart_fd, packet.bytes);
  }
  return 0;
}