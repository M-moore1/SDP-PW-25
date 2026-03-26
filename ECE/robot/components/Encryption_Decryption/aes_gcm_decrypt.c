// aes_gcm_decrypt.c
//
// Provides: aes_gcm_decrypt_packet()
// Optionally compiled as a standalone test binary when AES_GCM_MAIN is defined.
//
// ESP-IDF / WolfSSL build notes:
//   1. Enable WolfSSL in menuconfig:
//        Component config → ESP-wolfSSL → Enable wolfSSL
//   2. In your wolfssl/user_settings.h (or via menuconfig), ensure these are set:
//        #define HAVE_AESGCM
//        #define WOLFSSL_AES_256
//   3. Add this source file to your CMakeLists.txt component sources.
//
// Standalone host test (Linux, wolfssl installed):
//   gcc -DAES_GCM_MAIN aes_gcm_decrypt.c -o aes_gcm_test -lwolfssl

#include "aes_gcm_decrypt.h"

#include <string.h>
#include <stdio.h>
3
// WolfSSL on ESP-IDF: the component exposes headers under "wolfssl/"
// On a host build with an installed wolfssl package the same paths apply.
#include <wolfssl/wolfcrypt/aes.h>
#include <wolfssl/wolfcrypt/error-crypt.h>

// -------------------------------------------------------------------------
// Packet layout constants
// -------------------------------------------------------------------------
#define PACKET_LEN   156
#define NONCE_OFFSET   0
#define NONCE_LEN     12
#define CT_OFFSET     12
#define CT_LEN       128
#define TAG_OFFSET   140
#define TAG_LEN       16
// NONCE_LEN + CT_LEN + TAG_LEN == 156 ✓

// -------------------------------------------------------------------------
// Hard-coded AES-256 key (must be exactly 32 bytes).
// Replace with your actual key.
// -------------------------------------------------------------------------
// NOTE: This string is 64 hex chars but we use it as raw bytes — it is 64
// bytes long, not 32.  To get a 32-byte key either:
//   (a) use a 32-char ASCII secret directly, or
//   (b) hex-decode a 64-char hex string at runtime.
// Here we keep byte-for-byte parity with the original file: the string is
// used as raw bytes and the first 32 are taken as the AES-256 key.
// If you need a specific key value, replace AES_KEY_STR with your 32-char
// secret or adjust the hex-decode logic below.
// Replace the AES_KEY_STR / AES_KEY / AES_KEY_LEN block with this:

#define AES_KEY_HEX "a1b2c3d4e5f6789012345678901234567890abcdef1234567890abcdef123456"
#define AES_KEY_LEN 32

static uint8_t AES_KEY[AES_KEY_LEN];
static int aes_key_ready = 0;

static void init_aes_key(void) {
    if (aes_key_ready) return;
    const char *h = AES_KEY_HEX;
    for (int i = 0; i < AES_KEY_LEN; i++) {
        int hi = (h[2*i]   >= 'a') ? h[2*i]   - 'a' + 10 : h[2*i]   - '0';
        int lo = (h[2*i+1] >= 'a') ? h[2*i+1] - 'a' + 10 : h[2*i+1] - '0';
        AES_KEY[i] = (uint8_t)((hi << 4) | lo);
    }
    aes_key_ready = 1;
}

// -------------------------------------------------------------------------
// Optional AAD.  Set AAD_LEN to 0 if not used.
// -------------------------------------------------------------------------
static const uint8_t *AAD     = NULL;
static const int      AAD_LEN = 0;

// =========================================================================
// Public API
// =========================================================================

int aes_gcm_decrypt_packet(const uint8_t received_packet[PACKET_LEN],
                           char         *out_plaintext,
                           size_t       *out_len)
{
    init_aes_key();
    if (!received_packet || !out_plaintext || !out_len)
        return -1;

    // Pointers into the packet
    const uint8_t *nonce = received_packet + NONCE_OFFSET;
    const uint8_t *ct    = received_packet + CT_OFFSET;
    const uint8_t *tag   = received_packet + TAG_OFFSET;

    // ------------------------------------------------------------------
    // Initialise WolfSSL AES context and set key
    // ------------------------------------------------------------------
    Aes aes;
    int wc_ret = wc_AesInit(&aes, NULL, INVALID_DEVID);
    if (wc_ret != 0)
        return -1;

    wc_ret = wc_AesGcmSetKey(&aes, AES_KEY, AES_KEY_LEN);
    if (wc_ret != 0) {
        wc_AesFree(&aes);
        return -1;
    }

    // ------------------------------------------------------------------
    // Decrypt + verify tag in one call.
    //
    // wc_AesGcmDecrypt signature:
    //   int wc_AesGcmDecrypt(Aes*          aes,
    //                        byte*         out,        // plaintext output
    //                        const byte*   in,         // ciphertext input
    //                        word32        sz,         // ciphertext length
    //                        const byte*   iv,         // nonce
    //                        word32        ivSz,       // nonce length
    //                        const byte*   authTag,    // expected tag
    //                        word32        authTagSz,  // tag length
    //                        const byte*   authIn,     // AAD (may be NULL)
    //                        word32        authInSz);  // AAD length (0 if none)
    //
    // Returns 0 on success, AES_GCM_AUTH_E (-180) on authentication failure.
    // ------------------------------------------------------------------
    wc_ret = wc_AesGcmDecrypt(
        &aes,
        (uint8_t *)out_plaintext,   // plaintext output
        ct,                          // ciphertext input
        (word32)CT_LEN,              // ciphertext length
        nonce,                       // IV / nonce
        (word32)NONCE_LEN,           // nonce length
        tag,                         // expected GCM authentication tag
        (word32)TAG_LEN,             // tag length
        AAD,                         // additional authenticated data (or NULL)
        (word32)AAD_LEN              // AAD length (0 if none)
    );

    wc_AesFree(&aes);

    if (wc_ret == AES_GCM_AUTH_E) {
        return -2;  // authentication failure — mirrors the OpenSSL behaviour
    } else if (wc_ret != 0) {
        return -1;  // other wolfCrypt error
    }

    *out_len = (size_t)CT_LEN;

    // NUL-terminate so the caller can treat the buffer as a C string
    out_plaintext[*out_len] = '\0';

    return 0;
}

// =========================================================================
// Optional standalone test / demo  (compile with -DAES_GCM_MAIN)
// =========================================================================
#ifdef AES_GCM_MAIN

// ---------- tiny hex helpers (only needed for the test harness) ----------
static int from_hex(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static int hex_decode_into(const char *hex, uint8_t *out, size_t expected_len) {
    size_t n = strlen(hex);
    if (n != expected_len * 2) return 0;
    for (size_t i = 0; i < n; i += 2) {
        int hi = from_hex(hex[i]), lo = from_hex(hex[i + 1]);
        if (hi < 0 || lo < 0) return 0;
        out[i / 2] = (uint8_t)((hi << 4) | lo);
    }
    return 1;
}

int main(int argc, char **argv) {
    // Expect exactly one argument: 312 hex chars (156 bytes)
    if (argc != 2 || strlen(argv[1]) != PACKET_LEN * 2) {
        fprintf(stderr,
            "Usage: %s <HEX>\n"
            "  HEX must be exactly %d hex characters (%d bytes):\n"
            "  [nonce %d bytes][ciphertext %d bytes][tag %d bytes]\n",
            argv[0], PACKET_LEN * 2, PACKET_LEN,
            NONCE_LEN, CT_LEN, TAG_LEN);
        return 1;
    }

    uint8_t packet[PACKET_LEN];
    if (!hex_decode_into(argv[1], packet, PACKET_LEN)) {
        fprintf(stderr, "Invalid hex input.\n");
        return 1;
    }

    // out_plaintext: CT_LEN bytes + 1 for NUL
    char plaintext[CT_LEN + 1];
    size_t pt_len = 0;

    int ret = aes_gcm_decrypt_packet(packet, plaintext, &pt_len);

    if (ret == 0) {
        printf("Decrypted (%zu bytes): %s\n", pt_len, plaintext);
        return 0;
    } else if (ret == -2) {
        fprintf(stderr, "Authentication failed – bad key, corrupted packet, or wrong AAD.\n");
        return 2;
    } else {
        fprintf(stderr, "Decryption error (code %d).\n", ret);
        return 1;
    }
}
#endif /* AES_GCM_MAIN */