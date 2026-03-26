// aes_gcm_test_main.c
//
// Minimal round-trip test for aes_gcm_encrypt_packet() / aes_gcm_decrypt_packet().
//
// Build (Linux, wolfssl installed):
//   gcc aes_gcm_test_main.c aes_gcm_encrypt.c aes_gcm_decrypt.c -o aes_gcm_test -lwolfssl
//
// Build (ESP-IDF): add aes_gcm_test_main.c to your CMakeLists.txt sources
// and call aes_gcm_test() from app_main().

#include "aes_gcm_encrypt.h"
#include "aes_gcm_decrypt.h"

#include <stdio.h>
#include <string.h>

#define CT_LEN      128
#define PACKET_LEN  156

// ---- hardcoded test value -----------------------------------------------
static const char TEST_PLAINTEXT[] = "Hello ESP32! AES-256-GCM round-trip test.";

// =========================================================================

int aes_gcm_test(void)
{
    printf("=== AES-256-GCM round-trip test ===\n");
    printf("Input  : \"%s\"\n", TEST_PLAINTEXT);

    // --- build zero-padded 128-byte plaintext ----------------------------
    char plaintext_in[CT_LEN];
    memset(plaintext_in, 0, CT_LEN);
    memcpy(plaintext_in, TEST_PLAINTEXT, strlen(TEST_PLAINTEXT));

    // --- encrypt ---------------------------------------------------------
    uint8_t packet[PACKET_LEN];
    int ret = aes_gcm_encrypt_packet(plaintext_in, packet);
    if (ret != 0) {
        printf("FAIL: encrypt returned %d\n", ret);
        return ret;
    }
    printf("Encrypt: OK (nonce+ct+tag written, %d bytes)\n", PACKET_LEN);

    // --- decrypt ---------------------------------------------------------
    char plaintext_out[CT_LEN + 1];
    size_t out_len = 0;
    ret = aes_gcm_decrypt_packet(packet, plaintext_out, &out_len);
    if (ret == -2) {
        printf("FAIL: authentication error — tag mismatch\n");
        return ret;
    } else if (ret != 0) {
        printf("FAIL: decrypt returned %d\n", ret);
        return ret;
    }
    printf("Decrypt: OK (%zu bytes)\n", out_len);

    // --- verify ----------------------------------------------------------
    if (memcmp(plaintext_in, plaintext_out, CT_LEN) != 0) {
        printf("FAIL: plaintext mismatch after decryption\n");
        return -99;
    }

    printf("Output : \"%s\"\n", plaintext_out);
    printf("Result : PASS\n");
    return 0;
}

// ---- host entry point (not used on ESP-IDF) -----------------------------
#ifndef ESP_PLATFORM
int main(void) {
    return aes_gcm_test() == 0 ? 0 : 1;
}
#endif