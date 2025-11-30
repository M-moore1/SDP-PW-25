// aes_gcm_encrypt.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>

// Mason
static int from_hex(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static int hex_decode(const char *hex, unsigned char **out, size_t *out_len) {
    size_t n = strlen(hex);
    if (n % 2 != 0) return 0;
    *out = (unsigned char*)malloc(n/2);
    if (!*out) return 0;
    for (size_t i = 0; i < n; i += 2) {
        int hi = from_hex(hex[i]), lo = from_hex(hex[i+1]);
        if (hi < 0 || lo < 0) { free(*out); return 0; }
        (*out)[i/2] = (unsigned char)((hi<<4) | lo);
    }
    *out_len = n/2;
    return 1;
}
static char *hex_encode(const unsigned char *buf, size_t len) {
    static const char *digits = "0123456789abcdef";
    char *out = (char*)malloc(len*2 + 1);
    if (!out) return NULL;
    for (size_t i = 0; i < len; ++i) {
        out[i*2]   = digits[(buf[i] >> 4) & 0xF];
        out[i*2+1] = digits[buf[i] & 0xF];
    }
    out[len*2] = '\0';
    return out;
}
static unsigned char *read_all_stdin(size_t *len) {
    size_t cap = 4096, n = 0;
    unsigned char *buf = (unsigned char*)malloc(cap);
    if (!buf) return NULL;
    size_t r;
    while ((r = fread(buf + n, 1, cap - n, stdin)) > 0) {
        n += r;
        if (n == cap) {
            cap *= 2;
            unsigned char *tmp = (unsigned char*)realloc(buf, cap);
            if (!tmp) { free(buf); return NULL; }
            buf = tmp;
        }
    }
    *len = n;
    return buf;
}

int main(int argc, char **argv) {
    if (argc < 3 || argc > 4) {
        fprintf(stderr, "Usage: %s KEY_HEX(32B) NONCE_HEX(12B) [AAD_HEX]\n", argv[0]);
        return 1;
    }

    unsigned char *key=NULL,*nonce=NULL,*aad=NULL;
    size_t key_len=0, nonce_len=0, aad_len=0;

    if (!hex_decode(argv[1], &key, &key_len) || key_len != 32) {
        fprintf(stderr, "Key must be 32 bytes (64 hex chars)\n"); return 1;
    }
    if (!hex_decode(argv[2], &nonce, &nonce_len) || nonce_len != 12) {
        fprintf(stderr, "Nonce must be 12 bytes (24 hex chars)\n"); return 1;
    }
    if (argc == 4 && !hex_decode(argv[3], &aad, &aad_len)) {
        fprintf(stderr, "Invalid AAD hex\n"); return 1;
    }

    size_t pt_len=0;
    unsigned char *pt = read_all_stdin(&pt_len);
    if (!pt) { fprintf(stderr, "Failed to read stdin\n"); return 1; }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) { fprintf(stderr, "CTX new failed\n"); return 1; }

    int ok = EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL);
    ok &= EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, (int)nonce_len, NULL);
    ok &= EVP_EncryptInit_ex(ctx, NULL, NULL, key, nonce);
    if (!ok) { fprintf(stderr, "EncryptInit failed\n"); return 1; }

    int outl=0, tot=0;
    unsigned char *ct = (unsigned char*)malloc(pt_len + 16);
    if (!ct) { fprintf(stderr, "OOM\n"); return 1; }

    if (aad && aad_len > 0) {
        if (!EVP_EncryptUpdate(ctx, NULL, &outl, aad, (int)aad_len)) {
            fprintf(stderr, "AAD update failed\n"); return 1;
        }
    }
    if (!EVP_EncryptUpdate(ctx, ct, &outl, pt, (int)pt_len)) {
        fprintf(stderr, "EncryptUpdate failed\n"); return 1;
    }
    tot = outl;

    if (!EVP_EncryptFinal_ex(ctx, ct + tot, &outl)) {
        fprintf(stderr, "EncryptFinal failed\n"); return 1;
    }
    tot += outl;

    unsigned char tag[16];
    if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag)) {
        fprintf(stderr, "GET_TAG failed\n"); return 1;
    }

    char *ct_hex = hex_encode(ct, (size_t)tot);
    char *tag_hex = hex_encode(tag, 16);
    if (!ct_hex || !tag_hex) { fprintf(stderr, "hex encode failed\n"); return 1; }

    printf("CIPHERTEXT_HEX=%s\n", ct_hex);
    printf("TAG_HEX=%s\n", tag_hex);

    free(tag_hex);
    free(ct_hex);
    free(ct);
    EVP_CIPHER_CTX_free(ctx);
    free(pt);
    free(aad);
    free(nonce);
    free(key);
    return 0;
}
