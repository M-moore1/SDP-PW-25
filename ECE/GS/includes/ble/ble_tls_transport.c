#include "ble_tls_transport.h"
#include "pmod_esp32.h"

#include <string.h>

extern int ble_write(int uart_fd, int srv, int chr, int desc, uint8_t *data, int len);
extern int BLE_CONNECTED;

/*
 * Assumption:
 *   service index = 3
 *   characteristic index = 1
 * based on your existing code.
 *
 * If your TLS bytes should use a different characteristic than your robot command
 * path, change these values accordingly.
 */
#define TLS_SRV_INDEX   3
#define TLS_CHR_INDEX   1

static size_t ring_used(const BleTlsTransport *t) {
    if (t->rx_head >= t->rx_tail) {
        return t->rx_head - t->rx_tail;
    }
    return BLE_TLS_RX_BUF_SZ - t->rx_tail + t->rx_head;
}

static size_t ring_free(const BleTlsTransport *t) {
    /* keep one byte open so head==tail still means empty */
    return (BLE_TLS_RX_BUF_SZ - 1) - ring_used(t);
}

void ble_tls_transport_init(BleTlsTransport *t, int uart_fd) {
    if (!t) return;
    memset(t, 0, sizeof(*t));
    t->uart_fd = uart_fd;
}

int ble_tls_transport_feed_rx(BleTlsTransport *t, const uint8_t *data, size_t len) {
    if (!t || !data) return -1;
    if (len > ring_free(t)) return -2;  /* overflow */

    for (size_t i = 0; i < len; i++) {
        t->rx_buf[t->rx_head] = data[i];
        t->rx_head = (t->rx_head + 1) % BLE_TLS_RX_BUF_SZ;
    }
    return (int)len;
}

int ble_tls_transport_read(BleTlsTransport *t, uint8_t *out, size_t out_len) {
    if (!t || !out) return -1;

    size_t available = ring_used(t);
    if (available == 0) return 0;

    size_t n = (available < out_len) ? available : out_len;
    for (size_t i = 0; i < n; i++) {
        out[i] = t->rx_buf[t->rx_tail];
        t->rx_tail = (t->rx_tail + 1) % BLE_TLS_RX_BUF_SZ;
    }

    return (int)n;
}

int ble_tls_transport_write(BleTlsTransport *t, const uint8_t *data, size_t len) {
    if (!t || !data) return -1;
    if (!BLE_CONNECTED) return -2;

    size_t offset = 0;
    while (offset < len) {
        size_t chunk = len - offset;
        if (chunk > BLE_TLS_MAX_CHUNK) {
            chunk = BLE_TLS_MAX_CHUNK;
        }

        /*
         * ble_write sends one GATT write.
         * wolfSSL gives us a byte stream, so we fragment it here.
         */
        int ret = ble_write(
            t->uart_fd,
            TLS_SRV_INDEX,
            TLS_CHR_INDEX,
            -1,
            (uint8_t *)(data + offset),
            (int)chunk
        );

        if (ret < 0) {
            return ret;
        }

        offset += chunk;
    }

    return (int)len;
}