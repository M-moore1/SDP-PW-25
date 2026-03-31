#ifndef SOFTWARE_CRYPTOGRAPHY_H
#define SOFTWARE_CRYPTOGRAPHY_H

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/if_alg.h>
#include <linux/socket.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <stdlib.h>

#ifndef SOL_ALG
#define SOL_ALG 279
#endif

#define IV_SZ               12
#define TAG_SZ              16
#define CT_SZ               128
#define TOTAL_SZ            (IV_SZ + CT_SZ + TAG_SZ)   /* 156 bytes        */
#define PAYLOAD_HEX_STR_LEN (TOTAL_SZ * 2)             /* 312 hex chars    */
#define PAD_BYTE            0xFF
#define KEY_SIZE            32

int strip_pad(const uint8_t *buf, int len);
int sw_encryption(const char *Plaintext, char Ciphertext[PAYLOAD_HEX_STR_LEN + 1]);
int sw_decryption (const char *Ciphertext, uint8_t output[CT_SZ + 1]);
int encrypt_instr(const uint8_t instruction[8], char encrypted_string[PAYLOAD_HEX_STR_LEN+1]);

int gs_sw_crypto_init();
void gs_sw_crypto_deinit();

#endif