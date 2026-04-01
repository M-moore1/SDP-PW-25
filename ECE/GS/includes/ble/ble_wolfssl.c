#include "ble_wolfssl.h"

#include <string.h>

/*
 * wolfSSL custom I/O callbacks:
 *   int (*CallbackIORecv)(WOLFSSL *ssl, char *buf, int sz, void *ctx);
 *   int (*CallbackIOSend)(WOLFSSL *ssl, char *buf, int sz, void *ctx);
 *
 * Registered with wolfSSL_SetIORecv() and wolfSSL_SetIOSend().
 */

static int ble_io_send(WOLFSSL *ssl, char *buf, int sz, void *ctx) {
    (void)ssl;

    BleTlsTransport *t = (BleTlsTransport *)ctx;
    if (!t || !buf || sz <= 0) {
        return WOLFSSL_CBIO_ERR_GENERAL;
    }

    int ret = ble_tls_transport_write(t, (const uint8_t *)buf, (size_t)sz);
    if (ret < 0) {
        return WOLFSSL_CBIO_ERR_GENERAL;
    }

    return ret;
}

static int ble_io_recv(WOLFSSL *ssl, char *buf, int sz, void *ctx) {
    (void)ssl;

    BleTlsTransport *t = (BleTlsTransport *)ctx;
    if (!t || !buf || sz <= 0) {
        return WOLFSSL_CBIO_ERR_GENERAL;
    }

    int ret = ble_tls_transport_read(t, (uint8_t *)buf, (size_t)sz);

    /*
     * For nonblocking behavior, return WANT_READ when no bytes are available.
     * wolfSSL will retry later.
     */
    if (ret == 0) {
        return WOLFSSL_CBIO_ERR_WANT_READ;
    }
    if (ret < 0) {
        return WOLFSSL_CBIO_ERR_GENERAL;
    }

    return ret;
}

int ble_wolfssl_client_init(BleWolfSslClient *c, BleTlsTransport *transport) {
    if (!c || !transport) return -1;

    memset(c, 0, sizeof(*c));
    c->transport = transport;

    if (wolfSSL_Init() != WOLFSSL_SUCCESS) {
        return -2;
    }

    /*
     * TLS client method.
     * If you later decide to use DTLS instead, this is the place to switch.
     */
    c->ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method());
    if (!c->ctx) {
        return -3;
    }

    /*
     * Register custom transport I/O callbacks.
     */
    wolfSSL_SetIOSend(c->ctx, ble_io_send);
    wolfSSL_SetIORecv(c->ctx, ble_io_recv);

    /*
     * For real security, load a CA or pin a peer certificate/public key.
     * During initial bring-up, some people disable verification temporarily.
     * Do NOT leave that in production.
     */
    /* Example temporary testing only:
       wolfSSL_CTX_set_verify(c->ctx, SSL_VERIFY_NONE, NULL);
    */

    c->ssl = wolfSSL_new(c->ctx);
    if (!c->ssl) {
        wolfSSL_CTX_free(c->ctx);
        c->ctx = NULL;
        return -4;
    }

    wolfSSL_SetIOReadCtx(c->ssl, transport);
    wolfSSL_SetIOWriteCtx(c->ssl, transport);

    /* Tell wolfSSL the transport behaves nonblocking */
    wolfSSL_set_using_nonblock(c->ssl, 1);

    return 0;
}

int ble_wolfssl_client_handshake(BleWolfSslClient *c) {
    if (!c || !c->ssl) return -1;

    for (;;) {
        int ret = wolfSSL_connect(c->ssl);
        if (ret == WOLFSSL_SUCCESS) {
            return 0;
        }

        int err = wolfSSL_get_error(c->ssl, ret);

        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            /*
             * Caller should keep pumping BLE RX notifications into the transport.
             * In a real event loop, return a retry code instead of spinning.
             */
            continue;
        }

        return -2;
    }
}

int ble_wolfssl_client_write(BleWolfSslClient *c, const uint8_t *data, int len) {
    if (!c || !c->ssl || !data || len <= 0) return -1;

    for (;;) {
        int ret = wolfSSL_write(c->ssl, data, len);
        if (ret > 0) {
            return ret;
        }

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
        if (ret > 0) {
            return ret;
        }

        int err = wolfSSL_get_error(c->ssl, ret);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            continue;
        }

        return -2;
    }
}

void ble_wolfssl_client_cleanup(BleWolfSslClient *c) {
    if (!c) return;

    if (c->ssl) {
        wolfSSL_free(c->ssl);
        c->ssl = NULL;
    }
    if (c->ctx) {
        wolfSSL_CTX_free(c->ctx);
        c->ctx = NULL;
    }

    wolfSSL_Cleanup();
}