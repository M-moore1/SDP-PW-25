// test_aes_gcm.c
// Simple tests for aes_gcm_encrypt and aes_gcm_decrypt CLI tools.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char g_ct_hex[4096];
static char g_tag_hex[256];

static int run_encrypt_and_capture(void) {
    const char *key_hex =
        "00112233445566778899aabbccddeeff"
        "00112233445566778899aabbccddeeff"; // 32 bytes (256-bit key)
    const char *nonce_hex = "00112233445566778899aabb";          // 12 bytes
    const char *plaintext = "Hello, AES-GCM test!";

    // 1) Write plaintext to a file (pt.txt)
    FILE *f = fopen("pt.txt", "w");
    if (!f) {
        perror("fopen pt.txt");
        return 0;
    }
    fputs(plaintext, f);
    fclose(f);

    // 2) Run aes_gcm_encrypt, redirecting stdin/stdout via files.
    //    ./aes_gcm_encrypt KEY NONCE < pt.txt > enc_out.txt
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
             "./aes_gcm_encrypt %s %s < pt.txt > enc_out.txt",
             key_hex, nonce_hex);
    int rc = system(cmd);
    if (rc != 0) {
        fprintf(stderr, "Encrypt command failed (rc=%d)\n", rc);
        return 0;
    }

    // 3) Parse enc_out.txt to extract CIPHERTEXT_HEX and TAG_HEX.
    f = fopen("enc_out.txt", "r");
    if (!f) {
        perror("fopen enc_out.txt");
        return 0;
    }

    char line[4096];
    g_ct_hex[0] = '\0';
    g_tag_hex[0] = '\0';

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "CIPHERTEXT_HEX=", 15) == 0) {
            sscanf(line + 15, "%4095s", g_ct_hex);
        } else if (strncmp(line, "TAG_HEX=", 8) == 0) {
            sscanf(line + 8, "%255s", g_tag_hex);
        }
    }
    fclose(f);

    if (g_ct_hex[0] == '\0' || g_tag_hex[0] == '\0') {
        fprintf(stderr, "Failed to parse ciphertext/tag from enc_out.txt\n");
        return 0;
    }

    printf("Encryption step passed. Ciphertext and tag captured.\n");
    return 1;
}

static int test_roundtrip_decryption(void) {
    const char *key_hex =
        "00112233445566778899aabbccddeeff"
        "00112233445566778899aabbccddeeff";
    const char *nonce_hex = "00112233445566778899aabb";
    const char *expected_plaintext = "Hello, AES-GCM test!";

    // Write ciphertext hex into ct.txt
    FILE *f = fopen("ct.txt", "w");
    if (!f) {
        perror("fopen ct.txt");
        return 0;
    }
    fputs(g_ct_hex, f);
    fputc('\n', f);
    fclose(f);

    // ./aes_gcm_decrypt KEY NONCE TAG < ct.txt > dec_out.txt
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
             "./aes_gcm_decrypt %s %s %s < ct.txt > dec_out.txt",
             key_hex, nonce_hex, g_tag_hex);
    int rc = system(cmd);
    if (rc != 0) {
        fprintf(stderr, "Decrypt command failed (rc=%d)\n", rc);
        return 0;
    }

    // Read decrypted plaintext.
    f = fopen("dec_out.txt", "r");
    if (!f) {
        perror("fopen dec_out.txt");
        return 0;
    }
    char buf[4096];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    buf[n] = '\0';

    // Trim trailing newline / carriage return characters.
    while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r')) {
        buf[--n] = '\0';
    }

    size_t exp_len = strlen(expected_plaintext);
    if (n != exp_len || memcmp(buf, expected_plaintext, exp_len) != 0) {
        fprintf(stderr, "Roundtrip mismatch.\nExpected: %s\nGot:      %s\n",
                expected_plaintext, buf);
        return 0;
    }

    printf("Roundtrip encryption/decryption test PASSED.\n");
    return 1;
}


static int test_tampered_tag_fails(void) {
    const char *key_hex =
        "00112233445566778899aabbccddeeff"
        "00112233445566778899aabbccddeeff";
    const char *nonce_hex = "00112233445566778899aabb";

    // Make a slightly modified copy of the tag.
    char bad_tag[256];
    strncpy(bad_tag, g_tag_hex, sizeof(bad_tag) - 1);
    bad_tag[sizeof(bad_tag) - 1] = '\0';

    if (bad_tag[0] == '0')
        bad_tag[0] = '1';
    else
        bad_tag[0] = '0';

    // Re-use ct.txt (already contains ciphertext hex).
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
             "./aes_gcm_decrypt %s %s %s < ct.txt > dec_tampered.txt",
             key_hex, nonce_hex, bad_tag);

    int rc = system(cmd);

    if (rc == 0) {
        fprintf(stderr, "Tampered tag unexpectedly succeeded!\n");
        return 0;
    }

    printf("Tampered tag test PASSED (decryption failed as expected).\n");
    return 1;
}

int main(void) {
    int ok = 1;

    // Encrypts the known plaintext using the AES-GCM CLI tool and captures ciphertext and tag.
    ok &= run_encrypt_and_capture();
    if (!ok) {
        fprintf(stderr, "Encryption step failed. Aborting tests.\n");
        return 1;
    }

    // Verifies that decrypting the captured ciphertext produces the original plaintext (successful roundtrip).
    ok &= test_roundtrip_decryption();

    // Ensures authentication works by tampering with the tag and confirming decryption fails as expected.
    ok &= test_tampered_tag_fails();

    // Reports final test status, confirming whether all AES-GCM operations behaved correctly.
    if (!ok) {
        fprintf(stderr, "One or more tests FAILED.\n");
        return 1;
    }

    printf("All AES-GCM CLI tests PASSED.\n");
    return 0;
}
