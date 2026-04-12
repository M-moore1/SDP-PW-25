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

#include "../cmd_structure.h"

int strip_pad(const uint8_t *buf, int len);

int encrypt_json(const char *Plaintext, uint8_t Ciphertext[TOTAL_SZ]);
int encrypt_cmd(const robot_bt_packet_t *packet, uint8_t *cipher_out, size_t *cipher_out_len);

int decrypt_json(const uint8_t *encrypted, char *json_out, size_t json_out_len);
int decrypt_cmd(const uint8_t *encrypted, robot_bt_packet_t *pkt_out);


int gs_sw_crypto_init();
void gs_sw_crypto_deinit();

#endif