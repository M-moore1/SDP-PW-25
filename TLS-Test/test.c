#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>

/* Local TCP port where the TLS server will listen. */
#define PORT 4433
/* PEM certificate presented by this server during TLS handshake. */
#define CERT_FILE "server-cert.pem"
/* PEM private key that matches CERT_FILE. */
#define KEY_FILE "server-key.pem"

static int create_listen_socket(void) {
    /* Stage 1: Create a TCP socket endpoint. */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }

    /* Stage 2: Allow immediate restart after program exit (avoids TIME_WAIT bind issues). */
    int reuse = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt");
        close(sockfd);
        return -1;
    }

    /* Stage 3: Define bind address (0.0.0.0:PORT). */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);

    /* Stage 4: Bind socket to the configured local address/port. */
    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(sockfd);
        return -1;
    }

    /* Stage 5: Begin listening for incoming TCP connections. */
    if (listen(sockfd, 5) < 0) {
        perror("listen");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

int main(void) {
    /* File descriptors and TLS objects are initialized for safe cleanup later. */
    int listen_fd = -1;
    int client_fd = -1;
    WOLFSSL_CTX *ctx = NULL;
    WOLFSSL *ssl = NULL;

    /* Stage 0: Initialize global wolfSSL state. Must be called before any TLS API usage. */
    wolfSSL_Init();

    /* Stage 1: Create a server-side TLS context (TLS 1.2 method). */
    ctx = wolfSSL_CTX_new(wolfTLSv1_2_server_method());
    if (ctx == NULL) {
        fprintf(stderr, "wolfSSL_CTX_new failed\n");
        goto cleanup;
    }

    /* Stage 2: Load server certificate chain into TLS context. */
    if (wolfSSL_CTX_use_certificate_file(ctx, CERT_FILE, SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        fprintf(stderr, "Failed loading cert: %s\n", CERT_FILE);
        goto cleanup;
    }

    /* Stage 3: Load private key corresponding to the server certificate. */
    if (wolfSSL_CTX_use_PrivateKey_file(ctx, KEY_FILE, SSL_FILETYPE_PEM) != SSL_SUCCESS) {
        fprintf(stderr, "Failed loading key: %s\n", KEY_FILE);
        goto cleanup;
    }

    /* Stage 4: Prepare listening TCP socket. */
    listen_fd = create_listen_socket();
    if (listen_fd < 0) {
        goto cleanup;
    }

    printf("wolfSSL server listening on 127.0.0.1:%d\n", PORT);

    /* Stage 5: Block until a client establishes a TCP connection. */
    client_fd = accept(listen_fd, NULL, NULL);
    if (client_fd < 0) {
        perror("accept");
        goto cleanup;
    }

    /* Stage 6: Create per-connection TLS session object from the shared context. */
    ssl = wolfSSL_new(ctx);
    if (ssl == NULL) {
        fprintf(stderr, "wolfSSL_new failed\n");
        goto cleanup;
    }

    /* Stage 7: Attach accepted TCP socket to TLS session object. */
    if (wolfSSL_set_fd(ssl, client_fd) != SSL_SUCCESS) {
        fprintf(stderr, "wolfSSL_set_fd failed\n");
        goto cleanup;
    }

    /* Stage 8: Perform server-side TLS handshake. */
    if (wolfSSL_accept(ssl) != SSL_SUCCESS) {
        int err = wolfSSL_get_error(ssl, 0);
        fprintf(stderr, "TLS handshake failed: %d\n", err);
        goto cleanup;
    }

    /* Stage 9: Receive encrypted application data over the established TLS channel. */
    char buf[1024];
    int n = wolfSSL_read(ssl, buf, (int)sizeof(buf) - 1);
    if (n > 0) {
        /* Null-terminate so received bytes can be printed as a C string. */
        buf[n] = '\0';
        printf("Received from client: %s\n", buf);
    } else {
        int err = wolfSSL_get_error(ssl, n);
        fprintf(stderr, "wolfSSL_read failed: %d\n", err);
        goto cleanup;
    }

    /* Stage 10: Send encrypted response back to the client. */
    const char *reply = "Hello from wolfSSL C server";
    if (wolfSSL_write(ssl, reply, (int)strlen(reply)) <= 0) {
        int err = wolfSSL_get_error(ssl, 0);
        fprintf(stderr, "wolfSSL_write failed: %d\n", err);
        goto cleanup;
    }

    printf("Reply sent to client.\n");

cleanup:
    /* Stage 11: Clean shutdown path for both success and failure cases. */
    if (ssl != NULL) {
        /* Send TLS close_notify alert before freeing session resources. */
        wolfSSL_shutdown(ssl);
        wolfSSL_free(ssl);
    }
    if (client_fd >= 0) {
        /* Close connected client socket. */
        close(client_fd);
    }
    if (listen_fd >= 0) {
        /* Close listening socket. */
        close(listen_fd);
    }
    if (ctx != NULL) {
        /* Free TLS context shared by sessions. */
        wolfSSL_CTX_free(ctx);
    }
    /* Tear down global wolfSSL state. */
    wolfSSL_Cleanup();
    return 0;
}
