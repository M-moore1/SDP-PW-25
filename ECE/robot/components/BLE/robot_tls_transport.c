#include "robot_tls_transport.h"
#include "Robot_BLE.h"
#include <string.h>

extern uint16_t robot_conn_id;
extern esp_gatt_if_t robot_gatts_if;
extern bool device_connected;
extern bool notify_enabled;
extern uint16_t robot_handle_table[];

static RobotTlsTransport g_robot_tls_transport;

static size_t ring_used(const RobotTlsTransport *t) {
    if (t->rx_head >= t->rx_tail) return t->rx_head - t->rx_tail;
    return ROBOT_TLS_RX_BUF_SZ - t->rx_tail + t->rx_head;
}

static size_t ring_free(const RobotTlsTransport *t) {
    return (ROBOT_TLS_RX_BUF_SZ - 1) - ring_used(t);
}

void robot_tls_transport_init(RobotTlsTransport *t) {
    if (!t) return;
    memset(t, 0, sizeof(*t));
}

int robot_tls_transport_feed_rx(RobotTlsTransport *t, const uint8_t *data, size_t len) {
    if (!t || !data) return -1;
    if (len > ring_free(t)) return -2;

    for (size_t i = 0; i < len; i++) {
        t->rx_buf[t->rx_head] = data[i];
        t->rx_head = (t->rx_head + 1) % ROBOT_TLS_RX_BUF_SZ;
    }
    return (int)len;
}

int robot_tls_transport_read(RobotTlsTransport *t, uint8_t *out, size_t out_len) {
    if (!t || !out) return -1;

    size_t available = ring_used(t);
    if (available == 0) return 0;

    size_t n = (available < out_len) ? available : out_len;
    for (size_t i = 0; i < n; i++) {
        out[i] = t->rx_buf[t->rx_tail];
        t->rx_tail = (t->rx_tail + 1) % ROBOT_TLS_RX_BUF_SZ;
    }
    return (int)n;
}

int robot_tls_transport_write(const uint8_t *data, size_t len) {
    if (!data) return -1;
    if (!device_connected || !notify_enabled) return -2;

    esp_err_t err = esp_ble_gatts_send_indicate(
        robot_gatts_if,
        robot_conn_id,
        robot_handle_table[ROBOT_IDX_VAL],
        (uint16_t)len,
        (uint8_t *)data,
        false
    );

    return (err == ESP_OK) ? (int)len : -3;
}