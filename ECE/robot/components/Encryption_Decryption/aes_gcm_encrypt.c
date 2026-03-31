// aes_gcm_encrypt.c
//
// Provides: aes_gcm_encrypt_packet()
// Optionally compiled as a standalone test binary when AES_GCM_MAIN is defined.
//
// ESP-IDF / WolfSSL build notes:
//   1. Enable WolfSSL in menuconfig:
//        Component config → ESP-wolfSSL → Enable wolfSSL
//   2. In your wolfssl/user_settings.h (or via menuconfig), ensure these are set:
//        #define HAVE_AESGCM
//        #define WOLFSSL_AES_256
//        #define HAVE_HASHDRBG   // required for wc_RNG_GenerateBlock
//   3. Add this source file to your CMakeLists.txt component sources.
//
// Standalone host test (Linux, wolfssl installed):
//   gcc -DAES_GCM_MAIN aes_gcm_encrypt.c -o aes_gcm_test -lwolfssl

#include "aes_gcm_encrypt.h"

#include <string.h>
#include <stdio.h>

// WolfSSL on ESP-IDF: the component exposes headers under "wolfssl/"
// On a host build with an installed wolfssl package the same paths apply.
#include <wolfssl/wolfcrypt/aes.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/random.h>

// -------------------------------------------------------------------------
// Packet layout constants  (must mirror aes_gcm_decrypt.c exactly)
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
// Hard-coded AES-256 key — must be identical to the one in aes_gcm_decrypt.c
// -------------------------------------------------------------------------
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
// Optional AAD.  Must match the values in aes_gcm_decrypt.c.
// Set AAD_LEN to 0 if not used.
// -------------------------------------------------------------------------
static const uint8_t *AAD     = NULL;
static const int      AAD_LEN = 0;

// =========================================================================
// Public API
// =========================================================================

int aes_gcm_encrypt_packet(const char    *plaintext,
                           uint8_t        out_packet[PACKET_LEN])
{
    init_aes_key();
    if (!plaintext || !out_packet)
        return -1;

    // Pointers into the output packet
    uint8_t *nonce = out_packet + NONCE_OFFSET;
    uint8_t *ct    = out_packet + CT_OFFSET;
    uint8_t *tag   = out_packet + TAG_OFFSET;

    // ------------------------------------------------------------------
    // Generate a fresh random 12-byte nonce for every packet.
    // Re-using a nonce with the same key completely breaks GCM security.
    // ------------------------------------------------------------------
    WC_RNG rng;
    int wc_ret = wc_InitRng(&rng);
    if (wc_ret != 0)
        return -3;

    wc_ret = wc_RNG_GenerateBlock(&rng, nonce, NONCE_LEN);
    wc_FreeRng(&rng);
    if (wc_ret != 0)
        return -3;

    // ------------------------------------------------------------------
    // Initialise WolfSSL AES context and set key
    // ------------------------------------------------------------------
    Aes aes;
    wc_ret = wc_AesInit(&aes, NULL, INVALID_DEVID);
    if (wc_ret != 0)
        return -1;

    wc_ret = wc_AesGcmSetKey(&aes, AES_KEY, AES_KEY_LEN);
    if (wc_ret != 0) {
        wc_AesFree(&aes);
        return -1;
    }

    // ------------------------------------------------------------------
    // Encrypt + generate tag in one call.
    //
    // wc_AesGcmEncrypt signature:
    //   int wc_AesGcmEncrypt(Aes*          aes,
    //                        byte*         out,        // ciphertext output
    //                        const byte*   in,         // plaintext input
    //                        word32        sz,         // plaintext length
    //                        const byte*   iv,         // nonce
    //                        word32        ivSz,       // nonce length
    //                        byte*         authTag,    // tag output
    //                        word32        authTagSz,  // tag length
    //                        const byte*   authIn,     // AAD (may be NULL)
    //                        word32        authInSz);  // AAD length (0 if none)
    //
    // Returns 0 on success.
    // ------------------------------------------------------------------
    wc_ret = wc_AesGcmEncrypt(
        &aes,
        ct,                           // ciphertext output
        (const uint8_t *)plaintext,   // plaintext input
        (word32)CT_LEN,               // plaintext length
        nonce,                        // IV / nonce (written above)
        (word32)NONCE_LEN,            // nonce length
        tag,                          // GCM authentication tag output
        (word32)TAG_LEN,              // tag length
        AAD,                          // additional authenticated data (or NULL)
        (word32)AAD_LEN               // AAD length (0 if none)
    );

    wc_AesFree(&aes);

    if (wc_ret != 0)
        return -1;

    return 0;
}

// =========================================================================
// Optional standalone test / demo  (compile with -DAES_GCM_MAIN)
// =========================================================================
#ifdef AES_GCM_MAIN

// ---------- tiny hex helper (only needed for the test harness) -----------
static void hex_encode(const uint8_t *in, size_t len, char *out) {
    static const char lut[] = "0123456789abcdef";
    for (size_t i = 0; i < len; i++) {
        out[i * 2]     = lut[in[i] >> 4];
        out[i * 2 + 1] = lut[in[i] & 0x0f];
    }
    out[len * 2] = '\0';
}

int main(int argc, char **argv) {
    // Expect exactly one argument: up to 128 bytes of plaintext (as a string).
    if (argc != 2) {
        fprintf(stderr,
            "Usage: %s <PLAINTEXT>\n"
            "  PLAINTEXT will be zero-padded to %d bytes.\n"
            "  Output: %d hex chars (%d bytes):\n"
            "  [nonce %d bytes][ciphertext %d bytes][tag %d bytes]\n",
            argv[0], CT_LEN, PACKET_LEN * 2, PACKET_LEN,
            NONCE_LEN, CT_LEN, TAG_LEN);
        return 1;
    }

    // Build a zero-padded 128-byte plaintext buffer
    char plaintext[CT_LEN];
    memset(plaintext, 0, CT_LEN);
    size_t msglen = strlen(argv[1]);
    if (msglen > CT_LEN) {
        fprintf(stderr, "Plaintext too long (max %d bytes).\n", CT_LEN);
        return 1;
    }
    memcpy(plaintext, argv[1], msglen);

    uint8_t packet[PACKET_LEN];
    int ret = aes_gcm_encrypt_packet(plaintext, packet);

    if (ret == 0) {
        // Print the full packet as hex so it can be piped into the decrypt test
        char hex[PACKET_LEN * 2 + 1];
        hex_encode(packet, PACKET_LEN, hex);
        printf("%s\n", hex);
        return 0;
    } else if (ret == -3) {
        fprintf(stderr, "Nonce generation failed.\n");
        return 3;
    } else {
        fprintf(stderr, "Encryption error (code %d).\n", ret);
        return 1;
    }
}
#endif /* AES_GCM_MAIN */
