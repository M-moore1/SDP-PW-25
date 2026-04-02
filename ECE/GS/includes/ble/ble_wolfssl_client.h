#ifndef BLE_WOLFSSL_CLIENT_H
#define BLE_WOLFSSL_CLIENT_H

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include "ble_tls_transport.h"

typedef struct {
    WOLFSSL_CTX *ctx;
    WOLFSSL *ssl;
    BleTlsTransport *transport;
} BleWolfSslClient;

int ble_wolfssl_client_init(BleWolfSslClient *c, BleTlsTransport *transport);
int ble_wolfssl_client_handshake(BleWolfSslClient *c);
int ble_wolfssl_client_write(BleWolfSslClient *c, const uint8_t *data, int len);
int ble_wolfssl_client_read(BleWolfSslClient *c, uint8_t *buf, int len);
void ble_wolfssl_client_cleanup(BleWolfSslClient *c);

#endif