#ifndef ROBOT_TLS_TRANSPORT_H
#define ROBOT_TLS_TRANSPORT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define ROBOT_TLS_RX_BUF_SZ 8192

typedef struct {
    uint8_t rx_buf[ROBOT_TLS_RX_BUF_SZ];
    size_t rx_head;
    size_t rx_tail;
} RobotTlsTransport;

void robot_tls_transport_init(RobotTlsTransport *t);
int robot_tls_transport_feed_rx(RobotTlsTransport *t, const uint8_t *data, size_t len);
int robot_tls_transport_read(RobotTlsTransport *t, uint8_t *out, size_t out_len);
int robot_tls_transport_write(const uint8_t *data, size_t len);

#endif