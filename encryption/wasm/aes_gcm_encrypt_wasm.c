/**
 * AES-256-GCM encrypt for WebAssembly (Emscripten)
 * Compatible with aes_gcm_encrypt.c output format - uses mbedtls for WASM portability.
 * Exports: encrypt_aes_gcm_json(key_hex, nonce_hex, plaintext) -> JSON string
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <emscripten.h>
#include "mbedtls/gcm.h"

static int from_hex(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static int hex_decode(const char *hex, unsigned char **out, size_t *out_len) {
    size_t n = strlen(hex);
    if (n % 2 != 0) return 0;
    *out = (unsigned char *)malloc(n / 2);
    if (!*out) return 0;
    for (size_t i = 0; i < n; i += 2) {
        int hi = from_hex(hex[i]), lo = from_hex(hex[i + 1]);
        if (hi < 0 || lo < 0) {
            free(*out);
            *out = NULL;
            return 0;
        }
        (*out)[i / 2] = (unsigned char)((hi << 4) | lo);
    }
    *out_len = n / 2;
    return 1;
}

static char *hex_encode(const unsigned char *buf, size_t len) {
    static const char *digits = "0123456789abcdef";
    char *out = (char *)malloc(len * 2 + 1);
    if (!out) return NULL;
    for (size_t i = 0; i < len; ++i) {
        out[i * 2] = digits[(buf[i] >> 4) & 0xF];
        out[i * 2 + 1] = digits[buf[i] & 0xF];
    }
    out[len * 2] = '\0';
    return out;
}

/* Escape a string for JSON (escape backslash and quote) */
static char *json_escape(const char *s) {
    size_t n = strlen(s);
    size_t cap = n * 2 + 3; /* worst case: every char escaped + quotes + null */
    char *out = (char *)malloc(cap);
    if (!out) return NULL;
    size_t j = 0;
    out[j++] = '"';
    for (size_t i = 0; i < n; i++) {
        if (j + 4 >= cap) {
            cap *= 2;
            char *tmp = (char *)realloc(out, cap);
            if (!tmp) {
                free(out);
                return NULL;
            }
            out = tmp;
        }
        if (s[i] == '\\' || s[i] == '"') {
            out[j++] = '\\';
        }
        out[j++] = s[i];
    }
    out[j++] = '"';
    out[j] = '\0';
    return out;
}

/**
 * Encrypt plaintext with AES-256-GCM.
 * Returns JSON string: {"nonce":"...","ct":"...","tag":"..."}
 * Caller must free the returned string. Returns NULL on error.
 */
EMSCRIPTEN_KEEPALIVE
char *encrypt_aes_gcm_json(const char *key_hex, const char *nonce_hex, const char *plaintext) {
    unsigned char *key = NULL, *nonce = NULL;
    size_t key_len = 0, nonce_len = 0;

    if (!key_hex || !nonce_hex || !plaintext) return NULL;

    if (!hex_decode(key_hex, &key, &key_len) || key_len != 32) {
        if (key) free(key);
        return NULL;
    }
    if (!hex_decode(nonce_hex, &nonce, &nonce_len) || nonce_len != 12) {
        free(key);
        if (nonce) free(nonce);
        return NULL;
    }

    size_t pt_len = strlen(plaintext);
    unsigned char *ct = (unsigned char *)malloc(pt_len + 16);
    unsigned char tag[16];
    if (!ct) {
        free(key);
        free(nonce);
        return NULL;
    }

    mbedtls_gcm_context ctx;
    mbedtls_gcm_init(&ctx);

    int ret = mbedtls_gcm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, key, 256);
    if (ret != 0) {
        mbedtls_gcm_free(&ctx);
        free(ct);
        free(key);
        free(nonce);
        return NULL;
    }

    ret = mbedtls_gcm_crypt_and_tag(&ctx, MBEDTLS_GCM_ENCRYPT,
                                    (size_t)pt_len, nonce, 12,
                                    NULL, 0, (const unsigned char *)plaintext, ct,
                                    16, tag);

    mbedtls_gcm_free(&ctx);
    free(key);
    free(nonce);

    if (ret != 0) {
        free(ct);
        return NULL;
    }

    char *ct_hex = hex_encode(ct, pt_len);
    char *tag_hex = hex_encode(tag, 16);
    char *nonce_hex_escaped = json_escape(nonce_hex);
    free(ct);

    if (!ct_hex || !tag_hex || !nonce_hex_escaped) {
        free(ct_hex);
        free(tag_hex);
        free(nonce_hex_escaped);
        return NULL;
    }

    char *ct_escaped = json_escape(ct_hex);
    char *tag_escaped = json_escape(tag_hex);
    free(ct_hex);
    free(tag_hex);

    if (!ct_escaped || !tag_escaped) {
        free(nonce_hex_escaped);
        free(ct_escaped);
        free(tag_escaped);
        return NULL;
    }

    size_t buflen = strlen("{\"nonce\":") + strlen(nonce_hex_escaped) +
                    strlen(",\"ct\":") + strlen(ct_escaped) +
                    strlen(",\"tag\":") + strlen(tag_escaped) + strlen("}") + 1;
    char *json = (char *)malloc(buflen);
    if (!json) {
        free(nonce_hex_escaped);
        free(ct_escaped);
        free(tag_escaped);
        return NULL;
    }
    snprintf(json, buflen, "{\"nonce\":%s,\"ct\":%s,\"tag\":%s}",
             nonce_hex_escaped, ct_escaped, tag_escaped);

    free(nonce_hex_escaped);
    free(ct_escaped);
    free(tag_escaped);
    return json;
}

EMSCRIPTEN_KEEPALIVE
void free_string(char *s) {
    free(s);
}
