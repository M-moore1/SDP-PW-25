#ifndef ROBOT_WOLFSSL_SERVER_H
#define ROBOT_WOLFSSL_SERVER_H

#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
#include "robot_tls_transport.h"

typedef struct {
    WOLFSSL_CTX *ctx;
    WOLFSSL *ssl;
    RobotTlsTransport transport;
} RobotWolfSslServer;

int robot_wolfssl_server_init(RobotWolfSslServer *s);
int robot_wolfssl_server_do_handshake(RobotWolfSslServer *s);
int robot_wolfssl_server_read(RobotWolfSslServer *s, uint8_t *buf, int len);
int robot_wolfssl_server_write(RobotWolfSslServer *s, const uint8_t *buf, int len);
void robot_wolfssl_server_cleanup(RobotWolfSslServer *s);

/* Feed incoming BLE characteristic bytes into TLS transport */
int robot_wolfssl_server_feed_ciphertext(RobotWolfSslServer *s, const uint8_t *data, int len);

#endif