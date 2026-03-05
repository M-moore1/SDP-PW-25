#include "cmd_parser_ble.h"

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "json_uds.h"
#include "ble.h"

// --------------- helpers ---------------

static void uds_send_err(int uds_fd, const char *msg) {
  if (uds_fd < 0) return;
  char out[256];
  snprintf(out, sizeof(out), "{\"type\":\"ERR\",\"msg\":\"%s\"}", msg ? msg : "error");
  uds_send_json(uds_fd, out);
}

static void uds_send_ok(int uds_fd, const char *type, const char *msg) {
  if (uds_fd < 0) return;
  char out[256];
  if (msg && msg[0]) {
    snprintf(out, sizeof(out), "{\"type\":\"%s\",\"ok\":true,\"msg\":\"%s\"}", type, msg);
  } else {
    snprintf(out, sizeof(out), "{\"type\":\"%s\",\"ok\":true}", type);
  }
  uds_send_json(uds_fd, out);
}

static int parse_u64_from_json(cJSON *root, const char *key, uint64_t *out) {
  if (!root || !key || !out) return -1;

  cJSON *v = cJSON_GetObjectItemCaseSensitive(root, key);
  if (!v) return -1;

  if (cJSON_IsNumber(v)) {
    // cJSON stores numbers as double; safe up to 2^53 exactly.
    // If you need full u64 range, prefer hex string in JSON.
    double d = v->valuedouble;
    if (d < 0) return -1;
    *out = (uint64_t)d;
    return 0;
  }

  if (cJSON_IsString(v) && v->valuestring) {
    const char *s = v->valuestring;
    while (isspace((unsigned char)*s)) s++;

    int base = 10;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) base = 16;

    errno = 0;
    char *endp = NULL;
    unsigned long long ull = strtoull(s, &endp, base);
    if (errno != 0) return -1;
    if (endp == s) return -1;
    *out = (uint64_t)ull;
    return 0;
  }

  return -1;
}

static const char* parse_str_from_json(cJSON *root, const char *key) {
  cJSON *v = cJSON_GetObjectItemCaseSensitive(root, key);
  if (cJSON_IsString(v) && v->valuestring) return v->valuestring;
  return NULL;
}

static int streq_ci(const char *a, const char *b) {
  if (!a || !b) return 0;
  while (*a && *b) {
    if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
    a++; b++;
  }
  return (*a == '\0' && *b == '\0');
}

// --------------- main entrypoint ---------------

void handle_node_json(int uart_fd, int uds_fd, const char *json_str) {
  if (!json_str) {
    uds_send_err(uds_fd, "null json");
    return;
  }

  cJSON *root = cJSON_Parse(json_str);
  if (!root) {
    uds_send_err(uds_fd, "invalid json");
    return;
  }

  const char *type = parse_str_from_json(root, "type");
  const char *cmd  = parse_str_from_json(root, "cmd");

  // Allow either "type" or "cmd"
  const char *op = type ? type : cmd;
  if (!op) {
    cJSON_Delete(root);
    uds_send_err(uds_fd, "missing type/cmd");
    return;
  }

  // -------- CONNECT --------
  if (streq_ci(op, "CONNECT")) {
    const char *mac = parse_str_from_json(root, "mac");
    if (!mac) mac = parse_str_from_json(root, "address");

    if (!mac || !mac[0]) {
      // If the bridge already auto-connects at startup, let UI just request notify enable.
      // We'll treat missing MAC as "init notifications + report link".
      if (ble_client_init(uart_fd) != 0) { uds_send_err(uds_fd, "ble client init failed"); goto out; }
      if (ble_enable_notifications(uart_fd) != 0) { uds_send_err(uds_fd, "ble notify enable failed"); goto out; }
      uds_send_ok(uds_fd, "CONNECT", "no mac provided; notifications enabled");
      goto out;
    }

    int rc = ble_connect_mac(uart_fd, mac);
    if (rc != 0) {
      uds_send_err(uds_fd, "ble connect failed");
      goto out;
    }

    // Optional setup
    (void)ble_client_init(uart_fd);
    (void)ble_enable_notifications(uart_fd);

    uds_send_ok(uds_fd, "CONNECT", "connected");
    goto out;
  }

  // -------- DISCONNECT --------
  if (streq_ci(op, "DISCONNECT")) {
    int rc = ble_disconnect(uart_fd);
    if (rc != 0) { uds_send_err(uds_fd, "ble disconnect failed"); goto out; }
    uds_send_ok(uds_fd, "DISCONNECT", "disconnected");
    goto out;
  }

  // -------- LINK CHECK --------
  if (streq_ci(op, "GK") || streq_ci(op, "LINK") || streq_ci(op, "STATUS")) {
    int st = ble_connect_check(uart_fd); // 0 connected, 1 disconnected, -1 error
    if (st < 0) { uds_send_err(uds_fd, "ble link check failed"); goto out; }
    if (uds_fd >= 0) {
      char outmsg[128];
      snprintf(outmsg, sizeof(outmsg),
               "{\"type\":\"LINK\",\"connected\":%d}", (st == 0) ? 1 : 0);
      uds_send_json(uds_fd, outmsg);
    }
    goto out;
  }

  // -------- PLAINTEXT INSTRUCTION (u64) --------
  if (streq_ci(op, "INSTR") || streq_ci(op, "INSTRUCTION") || streq_ci(op, "SEND")) {
    uint64_t instr = 0;
    if (parse_u64_from_json(root, "instruction", &instr) != 0 &&
        parse_u64_from_json(root, "word", &instr) != 0 &&
        parse_u64_from_json(root, "u64", &instr) != 0) {
      uds_send_err(uds_fd, "missing instruction/word/u64");
      goto out;
    }

    if (uart_send_instruction_ble(uart_fd, instr) != 0) {
      uds_send_err(uds_fd, "ble send instruction failed");
      goto out;
    }

    uds_send_ok(uds_fd, "INSTR", "sent");
    goto out;
  }

  // -------- ENCRYPTED WORD (u64) --------
  if (streq_ci(op, "ENC_WORD") || streq_ci(op, "ENCRYPTED_WORD") || streq_ci(op, "EWORD")) {
    uint64_t word = 0;
    if (parse_u64_from_json(root, "word", &word) != 0 &&
        parse_u64_from_json(root, "instruction", &word) != 0 &&
        parse_u64_from_json(root, "u64", &word) != 0) {
      uds_send_err(uds_fd, "missing word/u64");
      goto out;
    }

    if (uart_send_encrypted_word_ble(uart_fd, word) != 0) {
      uds_send_err(uds_fd, "ble send encrypted word failed");
      goto out;
    }

    uds_send_ok(uds_fd, "ENC_WORD", "sent");
    goto out;
  }

  // Unknown op
  uds_send_err(uds_fd, "unknown command");

out:
  cJSON_Delete(root);
}
