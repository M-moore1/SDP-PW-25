#ifndef JSON_UDS_H
#define JSON_UDS_H

#define _GNU_SOURCE                     // Enables some GNU extensions (safe on Linux)
#include <arpa/inet.h>                  // htonl/ntohl for endian conversion
#include <errno.h>                      // errno and error codes
#include <fcntl.h>                      // open(), fcntl() flags
#include <stdint.h>                     // uint8_t/uint16_t/uint32_t/uint64_t
#include <stdio.h>                      // printf(), perror()
#include <stdlib.h>                     // malloc(), free(), getenv()
#include <string.h>                     // memset(), memcpy(), strncpy(), strcmp()
#include <sys/select.h>                 // select()
#include <sys/socket.h>                 // socket(), bind(), listen(), accept()
#include <sys/stat.h>                   // chmod()
#include <sys/un.h>                     // sockaddr_un for Unix domain sockets
#include <termios.h>                    // termios UART config
#include <unistd.h>                     // read(), write(), close(), unlink()
#include "cJSON.h"     


int read_full(int fd, void *buf, size_t n);
int uds_send_json(int fd, const char *json);
int json_get_u8(const cJSON *obj, const char *key, uint8_t *out, int minv, int maxv);
int json_get_u16(const cJSON *obj, const char *key, uint16_t *out, int minv, int maxv);
int json_get_u32(const cJSON *obj, const char *key, uint32_t *out);
int uds_server_listen(const char *path);
#endif