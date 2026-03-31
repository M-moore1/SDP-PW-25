#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "software_cryptography.h"

// gcc -O2 -Wall -Wextra test_sw.c software_cryptography.c -o test_sw.o

#define TEST_CIPHERTEXT "8BDF573D3D50AF81FAD29D955B91D6BC09FF99709565F114ACDA91469379B86EC63824B37339EEC7AA903929137BD367D00B0A99534505FB3E603E24D02D77B1DBECA6931B981D18AE316FA14F2B32B8CB778305174D6961FDD3BEA0BE071223C75C3856286788AAEA14BCCD93A3F76FD1DDDD58C824E3467B73FF81F4D4AFAD61DB4398F119542F136E2C6AAB3B68615BCE9D26E148F04226C4994C"

/*
int main(void)
{
    uint8_t first_decryption[CT_SZ + 1]  = {0};
    uint8_t second_decryption[CT_SZ + 1] = {0};
    char    re_encrypted_hex[PAYLOAD_HEX_STR_LEN + 1] = {0};

    // sw_encryption() expects a hex string — size: stripped bytes * 2 + NUL 
    char    plaintext_hex[CT_SZ * 2 + 1] = {0};

    // ── Phase 1: Decrypt the hardcoded ciphertext 
    printf("--- Phase 1: Initial Decryption ---\n");

    int len1 = sw_decryption(TEST_CIPHERTEXT, first_decryption);
    if (len1 < 0) {
        printf("ERROR: First decryption failed: %d\n", len1);
        return 1;
    }

    // Strip trailing 0xFF pad bytes so we only re-encrypt the real JSON 
    int stripped = strip_pad(first_decryption, len1);
    printf("Decrypted JSON: %.*s\n\n", stripped, (char *)first_decryption);

    // ── Convert raw decrypted bytes → hex string for sw_encryption ── 
    for (int i = 0; i < stripped; i++)
        sprintf(plaintext_hex + i * 2, "%02x", first_decryption[i]);
    plaintext_hex[stripped * 2] = '\0';

    // ── Phase 2: Re-encrypt (sw_encryption takes hex string, 2 args)
    printf("--- Phase 2: Re-Encryption ---\n");

    if (sw_encryption(plaintext_hex, re_encrypted_hex) != 0) {
        printf("ERROR: Re-encryption failed.\n");
        return 1;
    }
    printf("New Hex Generated: %.32s... (truncated)\n\n", re_encrypted_hex);

    // Phase 3: Decrypt the re-encrypted blob
    printf("--- Phase 3: Final Decryption ---\n");

    int len2 = sw_decryption(re_encrypted_hex, second_decryption);
    if (len2 < 0) {
        printf("ERROR: Second decryption failed: %d\n", len2);
        return 1;
    }
    printf("Verified JSON: %.*s\n\n",
           strip_pad(second_decryption, len2), (char *)second_decryption);

    // ── Final comparison: compare only the real JSON portion
    int stripped2 = strip_pad(second_decryption, len2);
    if (stripped == stripped2 &&
        memcmp(first_decryption, second_decryption, stripped) == 0)
        printf("✅ SUCCESS: Round-trip verification complete. Messages match!\n");
    else
        printf("❌ FAILURE: Message mismatch between encryption cycles.\n");

    return 0;
}
*/
int main(void)
{
    /* Random 8-byte test instruction */
    uint8_t instruction[8] = {0xA3, 0x7F, 0x1C, 0xD4, 0x56, 0xE9, 0x2B, 0x80};

    char    encrypted_string[PAYLOAD_HEX_STR_LEN + 1] = {0};
    uint8_t decrypted_raw[CT_SZ + 1]                  = {0};
    uint8_t recovered_instr[8]                         = {0};

    printf("=== encrypt_instr / decrypt_instr Round-Trip Test ===\n\n");

    /* ── Input ── */
    printf("Input (8 bytes): ");
    for (int i = 0; i < 8; i++) printf("%02X ", instruction[i]);
    printf("\n\n");

    /* ── Encrypt ── */
    printf("--- Encryption ---\n");
    if (encrypt_instr(instruction, encrypted_string) != 0) {
        printf("ERROR: encryption failed\n");
        return 1;
    }

    printf("IV        (%2d B): %.*s\n",   IV_SZ,  IV_SZ  * 2, encrypted_string);
    printf("Ciphertext(%3d B): %.*s\n",   CT_SZ,  CT_SZ  * 2, encrypted_string + IV_SZ * 2);
    printf("GCM Tag   (%2d B): %.*s\n\n", TAG_SZ, TAG_SZ * 2, encrypted_string + (IV_SZ + CT_SZ) * 2);
    printf("Full (312 chars):\n  %s\n\n", encrypted_string);

    /* ── Decrypt ── */
    printf("--- Decryption ---\n");
    int dec_ret = sw_decryption(encrypted_string, decrypted_raw);
    if (dec_ret < 0) {
        printf("ERROR: decryption failed: %d\n", dec_ret);
        return 1;
    }

    /* First 8 bytes of decrypted buffer are our instruction bytes */
    memcpy(recovered_instr, decrypted_raw, 8);

    printf("Recovered (8 bytes): ");
    for (int i = 0; i < 8; i++) printf("%02X ", recovered_instr[i]);
    printf("\n\n");

    /* ── Verify ── */
    printf("--- Verification ---\n");
    if (memcmp(instruction, recovered_instr, 8) == 0)
        printf("SUCCESS: Recovered bytes match original instruction\n");
    else
        printf("FAILURE: Byte mismatch\n");

    return 0;
}