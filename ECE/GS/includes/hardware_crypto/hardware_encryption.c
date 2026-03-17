#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "hardware_encryption.h"

#define AES_KEY_HEX "a1b2c3d4e5f6789012345678901234567890abcdef1234567890abcdef123456"
#define AES_KEY_LEN 8 // words

static uint32_t AES_KEY[AES_KEY_LEN];
static int aes_key_ready = 0;

/*
void convert_hex_to_uint32_8(const char *hex_str, uint32_t out_array[8]) {
    char temp[9]; 
    temp[8] = '\0';

    for (int i = 0; i < 8; i++) {
        memcpy(temp, &hex_str[i * 8], 8); 
        out_array[i] = (uint32_t)strtoul(temp, NULL, 16);
    }
}
*/
// Read register
uint32_t read_csu_reg(uint64_t addr) {
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "echo 0x%lX > %s", addr, CSU_CONFIG_REG);
    int ret = system(cmd);
    if(ret != 0) {
        perror("echo address");
        return 0xFFFFFFFF;
    }

    FILE *f = fopen(CSU_CONFIG_REG, "r");
    if(!f) {
        perror("fopen");
        return 0xFFFFFFFF;
    }

    uint32_t val;
    if(fscanf(f, "%x", &val) != 1) val = 0xFFFFFFFF;
    fclose(f);
    return val;
}

// Write register
int write_csu_reg(uint64_t addr, uint32_t mask, uint32_t value) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "echo 0x%lX 0x%X 0x%X > %s", addr, mask, value, CSU_CONFIG_REG);
    int ret = system(cmd);
    if(ret != 0) {
        perror("write command");
        return -1;
    }
    return 0;
}

// Read Write Registers Only
int reg_write_check(uint64_t addr, uint32_t mask, uint32_t value){
    write_csu_reg(addr, mask, value);
    return (read_csu_reg(addr) & mask) == (value & mask);

}

int gs_cryptography_init()
{

    // 1️⃣ Set DMA and AES as sources/destinations of each other
    if (write_csu_reg(CSU_SSS_CFG_ADDR, 0xFFFFFFFF, 0x000005A0) < 0) {
        printf("Error setting SSS config\n");
        return -1;
    }

    // 2️⃣ Set AES key source to KUP (default, just to be safe)
    if (write_csu_reg(AES_KEY_SRC, 0xFFFFFFFF, 0x0) < 0) {
        printf("Error setting AES key source to KUP\n");
        return -1;
    }

    // 3️⃣ Convert hex key to 8 uint32_t words
    convert_hex_to_uint32_8(AES_KEY_HEX, AES_KEY);

    // 4️⃣ Write key to KUP registers
    uint64_t reg_addrs[AES_KEY_LEN] = {
        AES_KUP_0_ADDR, AES_KUP_1_ADDR, AES_KUP_2_ADDR, AES_KUP_3_ADDR,
        AES_KUP_4_ADDR, AES_KUP_5_ADDR, AES_KUP_6_ADDR, AES_KUP_7_ADDR
    };

    for (int i = 0; i < AES_KEY_LEN; i++) {
        if (write_csu_reg(reg_addrs[i], 0xFFFFFFFF, AES_KEY[i]) < 0) {
            printf("Error writing KUP register %d\n", i);
            return -1;
        }
    }

    // 5️⃣ Trigger AES key load
    if (write_csu_reg(AES_KEY_LOAD, 0x1, 1) < 0) {
        printf("Error triggering key load\n");
        return -1;
    }

    // 6️⃣ Wait for key_init_done (poll for up to 5 seconds)
    printf("Waiting for key_init_done...\n");
    for (int i = 0; i < 5; i++) {
        uint32_t status = read_csu_reg(AES_STATUS_ADDR);
        uint32_t key_init_done = (status >> 4) & 0x1; // bit 4
        printf("Check %d: key_init_done = %u (status = 0x%08X)\n", i + 1, key_init_done, status);
        if (key_init_done) break;
        sleep(1);
    }

    printf("AES KUP key init complete (status = 0x%08X)\n", read_csu_reg(AES_STATUS_ADDR));
    return 0;
}