#include "cmd_parser.h"
#include "json_uds.h"
#include "aes_gcm_blob.h"
#include "gs_key.h"

volatile int security_level = 0;
volatile int connection_status = 0;
volatile int authorization_code = 0x3FF;

// Return codes:
//  0 = handled locally, do not forward packet to ESP32
//  1 = OK to forward packet to ESP32 as normal
//  2 = SYS query that should be replied to Node (optional)
static int sys_cmd(int uart_fd, system_format_t *sys_inst) {
  switch (sys_inst->instruction) {

    case SECURITY_LEVEL:
      // If this instruction is meant as "query current security level"
      // store it in the struct so caller can reply to Node if desired.
      sys_inst->specific = (uint32_t)security_level;
      return 2;

    case Connect_Reconnect:
      rn42_connect_mac(uart_fd, "004B1224B0A6");
      return 0; // local-only

    case DISCONNECT:
      rn42_disconnect(uart_fd);
      return 0; // local-only

    default:
      return 1; // forward unknown/other SYS commands to ESP32 (if any exist)
  }
}

void query_cmd(int uart_fd, query_format_t query_inst){

}

static int encrypt_word_to_blobhex(uint64_t word, char *out_hex, size_t out_cap) {
  uint8_t nonce[AES_GCM_NONCE_LEN];
  uint8_t tag[AES_GCM_TAG_LEN];
  uint8_t pt[8];
  uint8_t ct[8];

  // pack word to big-endian bytes
  for (int i = 0; i < 8; i++) pt[i] = (uint8_t)(word >> (56 - 8*i));

  if (aes_gcm_random_nonce(nonce) != 0) return -1;

  if (aes_gcm_encrypt_bytes(GS_AES_KEY, nonce, NULL, 0, pt, sizeof(pt), ct, tag) != 0) return -1;

  // Blob hex length for word-ct: (12+16+8)*2 + 1 = 73
  if (out_cap < 2*(AES_GCM_NONCE_LEN + AES_GCM_TAG_LEN + 8) + 1) return -1;

  aes_gcm_build_blob_hex(nonce, tag, ct, sizeof(ct), out_hex);
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

    int s_rc = sys_cmd(uart_fd, &packet.sys);

    if (s_rc == 0) {
      // connect/disconnect handled locally; do NOT send anything to ESP32
      cJSON_Delete(root);
      return 0;
    }

    if (s_rc == 2) {
      // Optional: reply back to Node with the current security level
      // If you don't have a defined JSON format yet, you can skip this reply.
      char resp[128];
      snprintf(resp, sizeof(resp),
              "{\"type\":\"S\",\"instruction\":%u,\"security_level\":%u}",
              (unsigned)packet.sys.instruction,
              (unsigned)packet.sys.specific);
      uds_send_json(uds_fd, resp);

      cJSON_Delete(root);
      return 0;
    }
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

  // TODO add priority Queue   
  if (security_level) {
    // encrypt the 8-byte packed word on the GS↔ESP32 link
    return uart_send_encrypted_word(uart_fd, packet.raw);
  }
  return uart_send_instruction(uart_fd, packet.raw);
    

  return 0;
}