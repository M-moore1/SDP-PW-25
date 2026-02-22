#include "json_uds.h"

// ------------------------- UDS framing utilities -------------------------
// Because sockets are byte streams, we send "len + JSON bytes" so receiver knows boundaries.

int read_full(int fd, void *buf, size_t n) {
  uint8_t *p = (uint8_t*)buf;                           // Byte pointer into buffer
  size_t got = 0;                                       // Bytes read so far
  while (got < n) {                                     // Until we read n bytes
    ssize_t r = read(fd, p + got, n - got);             // Try to read remaining bytes
    if (r == 0) return 0;                               // EOF -> peer disconnected
    if (r < 0) {                                        // Error
      if (errno == EINTR) continue;                     // Interrupted -> retry
      if (errno == EAGAIN || errno == EWOULDBLOCK) return -2; // Nonblocking: no data
      return -1;                                        // Real error
    }
    got += (size_t)r;                                   // Add bytes read
  }
  return 1;                                             // Success
}

int uds_send_json(int fd, const char *json) {
  uint32_t len = (uint32_t)strlen(json);                // Length of JSON string
  uint32_t len_be = htonl(len);                         // Convert to big-endian length
  if (write(fd, &len_be, 4) != 4) return -1;            // Write 4-byte length
  if (write(fd, json, len) != (ssize_t)len) return -1;  // Write JSON bytes
  return 0;                                             // Success
}

// ------------------------- JSON field helpers -------------------------
// These helpers validate fields exist and are in an allowed range.

int json_get_u8(const cJSON *obj, const char *key, uint8_t *out, int minv, int maxv) {
  cJSON *it = cJSON_GetObjectItemCaseSensitive((cJSON*)obj, key); // Look up key
  if (!cJSON_IsNumber(it)) return -1;                    // Must be a number
  int v = it->valueint;                                  // Read as int
  if (v < minv || v > maxv) return -2;                   // Range check
  *out = (uint8_t)v;                                     // Output
  return 0;                                              // Success
}

int json_get_u16(const cJSON *obj, const char *key, uint16_t *out, int minv, int maxv) {
  cJSON *it = cJSON_GetObjectItemCaseSensitive((cJSON*)obj, key); // Look up key
  if (!cJSON_IsNumber(it)) return -1;                    // Must be a number
  int v = it->valueint;                                  // Read as int
  if (v < minv || v > maxv) return -2;                   // Range check
  *out = (uint16_t)v;                                    // Output
  return 0;                                              // Success
}

int json_get_u32(const cJSON *obj, const char *key, uint32_t *out) {
  cJSON *it = cJSON_GetObjectItemCaseSensitive((cJSON*)obj, key); // Look up key
  if (!cJSON_IsNumber(it)) return -1;                    // Must be number
  double dv = it->valuedouble;                           // Read as double
  if (dv < 0 || dv > 4294967295.0) return -2;            // Range check for u32
  *out = (uint32_t)dv;                                   // Output as u32
  return 0;                                              // Success
}

// ------------------------- UDS server setup -------------------------
// Create a Unix domain socket server that waits for Node to connect.

int uds_server_listen(const char *path) {
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);                // Create UDS stream socket
  if (fd < 0) { perror("socket uds"); return -1; }         // Error check

  struct sockaddr_un addr;                                 // Address struct for UDS
  memset(&addr, 0, sizeof(addr));                          // Clear it
  addr.sun_family = AF_UNIX;                               // Unix domain socket
  strncpy(addr.sun_path, path, sizeof(addr.sun_path)-1);   // Copy path into struct

  unlink(path);                                            // Remove old socket file if it exists

  if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {// Bind socket file
    perror("bind uds");
    close(fd);
    return -1;
  }

  chmod(path, 0660);                                       // Allow owner/group access (adjust later)

  if (listen(fd, 1) < 0) {                                 // Listen with backlog=1 (single-client)
    perror("listen uds");
    close(fd);
    return -1;
  }

  return fd;                                               // Return listening socket fd
}
