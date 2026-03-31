#include "aes_gcm_decrypt.h"
#include "aes_gcm_encrypt.h"

#include <stdio.h>
#include <string.h>

#define CT_LEN      128
#define PACKET_LEN  156
#define NONCE_LEN    12
#define TAG_LEN      16

// ---- hardcoded test value -----------------------------------------------
static const char TEST_PLAINTEXT[] = "{\"type\":\"C\",\"forward\":1,\"backward\":0,\"left\":0,\"right\":0,\"speed\":50,\"priority_level\":1}";

// ---- hardcoded encrypted packet to test decryption ----------------------
// Paste a "Full packet" hex string from a previous run here.
// Leave as empty string "" to skip this test.
static const char TEST_PACKET_HEX[] = "8BDF573D3D50AF81FAD29D955B91D6BC09FF99709565F114ACDA91469379B86EC63824B37339EEC7AA903929137BD367D00B0A99534505FB3E603E24D02D77B1DBECA6931B981D18AE316FA14F2B32B8CB778305174D6961FDD3BEA0BE071223C75C3856286788AAEA14BCCD93A3F76FD1DDDD58C824E3467B73FF81F4D4AFAD61DB4398F119542F136E2C6AAB3B68615BCE9D26E148F04226C4994C";

// ---- helper: print a byte buffer as lowercase hex -----------------------
static void print_hex(const char *label, const uint8_t *buf, size_t len)
{
    printf("%s", label);
    for (size_t i = 0; i < len; i++)
        printf("%02x", buf[i]);
    printf("\n");
}

// ---- helper: decode hex string into byte buffer -------------------------
static int hex_decode(const char *hex, uint8_t *out, size_t expected_len)
{
    if (strlen(hex) != expected_len * 2) return 0;
    for (size_t i = 0; i < expected_len; i++) {
        int hi = hex[2*i];
        int lo = hex[2*i+1];
        hi = (hi >= 'a') ? hi-'a'+10 : (hi >= 'A') ? hi-'A'+10 : hi-'0';
        lo = (lo >= 'a') ? lo-'a'+10 : (lo >= 'A') ? lo-'A'+10 : lo-'0';
        out[i] = (uint8_t)((hi << 4) | lo);
    }
    return 1;
}

// =========================================================================

int app_main(void)
{
    printf("=== AES-256-GCM round-trip test ===\n");
    printf("Input  : \"%s\"\n\n", TEST_PLAINTEXT);

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

    // --- print encrypted components --------------------------------------
    print_hex("Nonce      : ", packet,                      NONCE_LEN);
    print_hex("Ciphertext : ", packet + NONCE_LEN,          CT_LEN);
    print_hex("Tag        : ", packet + NONCE_LEN + CT_LEN, TAG_LEN);
    print_hex("Full packet: ", packet,                      PACKET_LEN);
    printf("\n");

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

    // --- verify ----------------------------------------------------------
    if (memcmp(plaintext_in, plaintext_out, CT_LEN) != 0) {
        printf("FAIL: plaintext mismatch after decryption\n");
        return -99;
    }

    printf("Decrypted  : \"%s\"\n", plaintext_out);
    printf("Result     : PASS\n");

    // -----------------------------------------------------------------------
    // Hardcoded packet test — paste a hex packet from a previous run above
    // -----------------------------------------------------------------------
    if (strlen(TEST_PACKET_HEX) == 0) {
        printf("\n[Hardcoded packet test skipped — TEST_PACKET_HEX is empty]\n");
        return 0;
    }

    printf("\n=== Hardcoded packet decrypt test ===\n");
    uint8_t known_packet[PACKET_LEN];
    if (!hex_decode(TEST_PACKET_HEX, known_packet, PACKET_LEN)) {
        printf("FAIL: TEST_PACKET_HEX must be exactly %d hex chars\n", PACKET_LEN * 2);
        return -1;
    }

    char known_out[CT_LEN + 1];
    size_t known_len = 0;
    int kret = aes_gcm_decrypt_packet(known_packet, known_out, &known_len);
    if (kret == -2) {
        printf("FAIL: authentication error — wrong key, corrupted packet, or bad hex\n");
        return kret;
    } else if (kret != 0) {
        printf("FAIL: decrypt returned %d\n", kret);
        return kret;
    }
    //TESTING DATA
    printf("For Comparison: %s\n", TEST_PLAINTEXT);
    printf("Decrypted  : \"%s\"\n", known_out);
    printf("Result     : PASS\n");
    return 0;
}