#include "ble_wolfssl_client.h"
#include <string.h>

static int ble_io_send(WOLFSSL *ssl, char *buf, int sz, void *ctx) {
    (void)ssl;
    BleTlsTransport *t = (BleTlsTransport *)ctx;
    if (!t || !buf || sz <= 0) return WOLFSSL_CBIO_ERR_GENERAL;

    int ret = ble_tls_transport_write(t, (const uint8_t *)buf, (size_t)sz);
    return (ret < 0) ? WOLFSSL_CBIO_ERR_GENERAL : ret;
}

static int ble_io_recv(WOLFSSL *ssl, char *buf, int sz, void *ctx) {
    (void)ssl;
    BleTlsTransport *t = (BleTlsTransport *)ctx;
    if (!t || !buf || sz <= 0) return WOLFSSL_CBIO_ERR_GENERAL;

    int ret = ble_tls_transport_read(t, (uint8_t *)buf, (size_t)sz);
    if (ret == 0) return WOLFSSL_CBIO_ERR_WANT_READ;
    if (ret < 0) return WOLFSSL_CBIO_ERR_GENERAL;
    return ret;
}

int ble_wolfssl_client_init(BleWolfSslClient *c, BleTlsTransport *transport) {
    if (!c || !transport) return -1;

    memset(c, 0, sizeof(*c));
    c->transport = transport;

    if (wolfSSL_Init() != WOLFSSL_SUCCESS) return -2;

    c->ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method());
    if (!c->ctx) return -3;

    wolfSSL_SetIOSend(c->ctx, ble_io_send);
    wolfSSL_SetIORecv(c->ctx, ble_io_recv);

    /* For testing only. Replace with real validation later. */
    wolfSSL_CTX_set_verify(c->ctx, SSL_VERIFY_NONE, NULL);

    c->ssl = wolfSSL_new(c->ctx);
    if (!c->ssl) return -4;

    wolfSSL_SetIOReadCtx(c->ssl, transport);
    wolfSSL_SetIOWriteCtx(c->ssl, transport);
    wolfSSL_set_using_nonblock(c->ssl, 1);

    return 0;
}

int ble_wolfssl_client_handshake(BleWolfSslClient *c) {
    if (!c || !c->ssl) return -1;

    for (;;) {
        int ret = wolfSSL_connect(c->ssl);
        if (ret == WOLFSSL_SUCCESS) return 0;

        int err = wolfSSL_get_error(c->ssl, ret);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            continue;
        }
        return -2;
    }
}

int ble_wolfssl_client_write(BleWolfSslClient *c, const uint8_t *data, int len) {
    if (!c || !c->ssl || !data || len <= 0) return -1;

    for (;;) {
        int ret = wolfSSL_write(c->ssl, data, len);
        if (ret > 0) return ret;

        int err = wolfSSL_get_error(c->ssl, ret);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            continue;
        }
        return -2;
    }
}

int ble_wolfssl_client_read(BleWolfSslClient *c, uint8_t *buf, int len) {
    if (!c || !c->ssl || !buf || len <= 0) return -1;

    for (;;) {
        int ret = wolfSSL_read(c->ssl, buf, len);
        if (ret > 0) return ret;

        int err = wolfSSL_get_error(c->ssl, ret);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            continue;
        }
        return -2;
    }
}

void ble_wolfssl_client_cleanup(BleWolfSslClient *c) {
    if (!c) return;
    if (c->ssl) wolfSSL_free(c->ssl);
    if (c->ctx) wolfSSL_CTX_free(c->ctx);
    wolfSSL_Cleanup();
    memset(c, 0, sizeof(*c));
}