#ifndef BLE_TLS_TRANSPORT_H
#define BLE_TLS_TRANSPORT_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BLE_TLS_MAX_CHUNK 128
#define BLE_TLS_RX_BUF_SZ 8192

typedef struct {
    int uart_fd;

    uint8_t rx_buf[BLE_TLS_RX_BUF_SZ];
    size_t rx_head;
    size_t rx_tail;
} BleTlsTransport;

/* Transport lifecycle */
void ble_tls_transport_init(BleTlsTransport *t, int uart_fd);

/* Called by your BLE notification handler when encrypted bytes arrive */
int ble_tls_transport_feed_rx(BleTlsTransport *t, const uint8_t *data, size_t len);

/* Read bytes for wolfSSL */
int ble_tls_transport_read(BleTlsTransport *t, uint8_t *out, size_t out_len);

/* Write bytes for wolfSSL (fragments across BLE packets) */
int ble_tls_transport_write(BleTlsTransport *t, const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif