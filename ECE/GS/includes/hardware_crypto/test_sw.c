#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "software_cryptography.h"
#include "../cmd_structure.h"

// gcc -O2 -Wall -Wextra test_sw.c software_cryptography.c -I.../includes/cmd_structure -o test_sw.o

#define TEST_CIPHERTEXT "8BDF573D3D50AF81FAD29D955B91D6BC09FF99709565F114ACDA91469379B86EC63824B37339EEC7AA903929137BD367D00B0A99534505FB3E603E24D02D77B1DBECA6931B981D18AE316FA14F2B32B8CB778305174D6961FDD3BEA0BE071223C75C3856286788AAEA14BCCD93A3F76FD1DDDD58C824E3467B73FF81F4D4AFAD61DB4398F119542F136E2C6AAB3B68615BCE9D26E148F04226C4994C"

/*
int main(void)
{
   
    robot_bt_packet_t pkt;
    pkt.bytes[0] = 0xA3; pkt.bytes[1] = 0x7F;
    pkt.bytes[2] = 0x1C; pkt.bytes[3] = 0xD4;
    pkt.bytes[4] = 0x56; pkt.bytes[5] = 0xE9;
    pkt.bytes[6] = 0x2B; pkt.bytes[7] = 0x80;

    uint8_t           encrypted[TOTAL_SZ] = {0};
    size_t            encrypted_len       = 0;
    robot_bt_packet_t recovered           = {0};

    printf("=== encrypt_cmd / decrypt_cmd Round-Trip Test ===\n\n");

    printf("Input (8 bytes): ");
    for (int i = 0; i < 8; i++) printf("%02X ", pkt.bytes[i]);
    printf("\n\n");


    printf("--- Encryption ---\n");
    if (encrypt_cmd(&pkt, encrypted, &encrypted_len) != 0) {
        printf("ERROR: encryption failed\n");
        return 1;
    }

    printf("IV        (%2d B): ", IV_SZ);
    for (int i = 0; i < IV_SZ; i++)              printf("%02X ", encrypted[i]);
    printf("\nCiphertext(%3d B): ", CT_SZ);
    for (int i = 0; i < CT_SZ; i++)              printf("%02X ", encrypted[IV_SZ + i]);
    printf("\nGCM Tag   (%2d B): ", TAG_SZ);
    for (int i = 0; i < TAG_SZ; i++)             printf("%02X ", encrypted[IV_SZ + CT_SZ + i]);
    printf("\n\nFull (%zu bytes): ", encrypted_len);
    for (int i = 0; i < (int)encrypted_len; i++) printf("%02X ", encrypted[i]);
    printf("\n\n");

    printf("--- Decryption ---\n");
    int dec_ret = decrypt_cmd(encrypted, &recovered);
    if (dec_ret < 0) {
        printf("ERROR: decryption failed: %d\n", dec_ret);
        return 1;
    }

    printf("Recovered (8 bytes): ");
    for (int i = 0; i < 8; i++) printf("%02X ", recovered.bytes[i]);
    printf("\n\n");

    printf("--- Verification ---\n");
    if (memcmp(pkt.bytes, recovered.bytes, sizeof(robot_bt_packet_t)) == 0)
        printf("SUCCESS: Recovered packet matches original\n");
    else
        printf("FAILURE: Byte mismatch\n");

    return 0;
}
*/


int main(void)
{
    printf("=== Encryption / Decryption Test ===\n\n");

    /* ── Part 1: encrypt_json -> decrypt_json Round-Trip ── */
    printf("--- Part 1: encrypt_json -> decrypt_json Round-Trip ---\n");

    const char *test_json = "{\"cmd\":\"move\",\"x\":10,\"y\":5}";

    uint8_t encrypted[TOTAL_SZ]  = {0};
    char    json_out[CT_SZ + 1]  = {0};

    printf("Input JSON: %s\n\n", test_json);

    /* Encrypt */
    printf("--- Encryption (encrypt_json) ---\n");
    if (encrypt_json(test_json, encrypted) != 0) {
        printf("ERROR: encrypt_json failed\n");
        return 1;
    }

    printf("IV        (%2d B): ", IV_SZ);
    for (int i = 0; i < IV_SZ; i++)             printf("%02X ", encrypted[i]);
    printf("\nCiphertext(%3d B): ", CT_SZ);
    for (int i = 0; i < CT_SZ; i++)             printf("%02X ", encrypted[IV_SZ + i]);
    printf("\nGCM Tag   (%2d B): ", TAG_SZ);
    for (int i = 0; i < TAG_SZ; i++)            printf("%02X ", encrypted[IV_SZ + CT_SZ + i]);
    printf("\n\nFull (%d bytes): ", TOTAL_SZ);
    for (int i = 0; i < TOTAL_SZ; i++)          printf("%02X ", encrypted[i]);
    printf("\n\n");

    /* Decrypt */
    printf("--- Decryption (decrypt_json) ---\n");
    int dec_ret = decrypt_json(encrypted, json_out, sizeof(json_out));
    if (dec_ret < 0) {
        printf("ERROR: decrypt_json failed: %d\n", dec_ret);
        return 1;
    }

    printf("Recovered JSON: %s\n\n", json_out);

    printf("--- Verification ---\n");
    if (strcmp(test_json, json_out) == 0)
        printf("SUCCESS: Recovered JSON matches original\n\n");
    else
        printf("FAILURE: JSON mismatch\n\n");

    /* ── Part 2: TEST_CIPHERTEXT -> decrypt_json ── */
    printf("--- Part 2: Static TEST_CIPHERTEXT -> decrypt_json ---\n");

    /* Convert TEST_CIPHERTEXT hex string to raw 156 bytes */
    uint8_t test_raw[TOTAL_SZ] = {0};
    for (int i = 0; i < TOTAL_SZ; i++)
        sscanf(TEST_CIPHERTEXT + i * 2, "%02hhx", &test_raw[i]);

    printf("Static ciphertext (%d bytes):\n  ", TOTAL_SZ);
    for (int i = 0; i < TOTAL_SZ; i++) printf("%02X ", test_raw[i]);
    printf("\n\n");

    /* decrypt_json */
    char static_json[CT_SZ + 1] = {0};
    int static_ret = decrypt_json(test_raw, static_json, sizeof(static_json));
    if (static_ret < 0) {
        printf("ERROR: decrypt_json on TEST_CIPHERTEXT failed: %d\n", static_ret);
        return 1;
    }
    printf("Decrypted JSON: %s\n\n", static_json);

    /* decrypt_cmd */
    printf("--- Also as packet (decrypt_cmd) ---\n");
    robot_bt_packet_t pkt = {0};
    int cmd_ret = decrypt_cmd(test_raw, &pkt);
    if (cmd_ret < 0) {
        printf("ERROR: decrypt_cmd on TEST_CIPHERTEXT failed: %d\n", cmd_ret);
        return 1;
    }

    printf("Packet bytes: ");
    for (int i = 0; i < 8; i++) printf("%02X ", pkt.bytes[i]);
    printf("\n\n");

    printf("SUCCESS: All tests passed\n");

    return 0;
}