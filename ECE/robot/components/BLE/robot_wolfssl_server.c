#include "robot_wolfssl_server.h"
#include <string.h>

static int robot_io_send(WOLFSSL *ssl, char *buf, int sz, void *ctx) {
    (void)ssl;
    RobotTlsTransport *t = (RobotTlsTransport *)ctx;
    (void)t;
    int ret = robot_tls_transport_write((const uint8_t *)buf, (size_t)sz);
    return (ret < 0) ? WOLFSSL_CBIO_ERR_GENERAL : ret;
}

static int robot_io_recv(WOLFSSL *ssl, char *buf, int sz, void *ctx) {
    (void)ssl;
    RobotTlsTransport *t = (RobotTlsTransport *)ctx;
    if (!t || !buf || sz <= 0) return WOLFSSL_CBIO_ERR_GENERAL;

    int ret = robot_tls_transport_read(t, (uint8_t *)buf, (size_t)sz);
    if (ret == 0) return WOLFSSL_CBIO_ERR_WANT_READ;
    if (ret < 0) return WOLFSSL_CBIO_ERR_GENERAL;
    return ret;
}

int robot_wolfssl_server_init(RobotWolfSslServer *s) {
    if (!s) return -1;
    memset(s, 0, sizeof(*s));

    robot_tls_transport_init(&s->transport);

    if (wolfSSL_Init() != WOLFSSL_SUCCESS) return -2;

    s->ctx = wolfSSL_CTX_new(wolfTLSv1_2_server_method());
    if (!s->ctx) return -3;

    wolfSSL_SetIOSend(s->ctx, robot_io_send);
    wolfSSL_SetIORecv(s->ctx, robot_io_recv);

    /* Replace with your real cert/key files or buffers */
    if (wolfSSL_CTX_use_certificate_file(s->ctx, "/spiffs/server-cert.pem", SSL_FILETYPE_PEM) != SSL_SUCCESS)
        return -4;

    if (wolfSSL_CTX_use_PrivateKey_file(s->ctx, "/spiffs/server-key.pem", SSL_FILETYPE_PEM) != SSL_SUCCESS)
        return -5;

    s->ssl = wolfSSL_new(s->ctx);
    if (!s->ssl) return -6;

    wolfSSL_SetIOReadCtx(s->ssl, &s->transport);
    wolfSSL_SetIOWriteCtx(s->ssl, &s->transport);
    wolfSSL_set_using_nonblock(s->ssl, 1);

    return 0;
}

int robot_wolfssl_server_feed_ciphertext(RobotWolfSslServer *s, const uint8_t *data, int len) {
    if (!s || !data || len <= 0) return -1;
    return robot_tls_transport_feed_rx(&s->transport, data, (size_t)len);
}

int robot_wolfssl_server_do_handshake(RobotWolfSslServer *s) {
    if (!s || !s->ssl) return -1;

    for (;;) {
        int ret = wolfSSL_accept(s->ssl);
        if (ret == WOLFSSL_SUCCESS) return 0;

        int err = wolfSSL_get_error(s->ssl, ret);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            continue;
        }
        return -2;
    }
}

int robot_wolfssl_server_read(RobotWolfSslServer *s, uint8_t *buf, int len) {
    if (!s || !s->ssl || !buf || len <= 0) return -1;

    for (;;) {
        int ret = wolfSSL_read(s->ssl, buf, len);
        if (ret > 0) return ret;

        int err = wolfSSL_get_error(s->ssl, ret);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            continue;
        }
        return -2;
    }
}

int robot_wolfssl_server_write(RobotWolfSslServer *s, const uint8_t *buf, int len) {
    if (!s || !s->ssl || !buf || len <= 0) return -1;

    for (;;) {
        int ret = wolfSSL_write(s->ssl, buf, len);
        if (ret > 0) return ret;

        int err = wolfSSL_get_error(s->ssl, ret);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            continue;
        }
        return -2;
    }
}

void robot_wolfssl_server_cleanup(RobotWolfSslServer *s) {
    if (!s) return;
    if (s->ssl) wolfSSL_free(s->ssl);
    if (s->ctx) wolfSSL_CTX_free(s->ctx);
    wolfSSL_Cleanup();
    memset(s, 0, sizeof(*s));
}