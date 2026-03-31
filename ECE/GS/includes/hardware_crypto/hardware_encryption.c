#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "hardware_encryption.h"



void convert_hex_to_uint32_8(const char *hex_str, uint32_t out_array[8]) {
    char temp[9]; 
    temp[8] = '\0';

    for (int i = 0; i < 8; i++) {
        memcpy(temp, &hex_str[i * 8], 8); 
        out_array[i] = (uint32_t)strtoul(temp, NULL, 16);
    }
}

// Read register
uint32_t read_csu_reg(uint64_t addr) {
    char cmd[128];

    /* Step 1: Write address to config_reg */
    snprintf(cmd, sizeof(cmd), "echo 0x%lX > %s", addr, CSU_CONFIG_REG);
    if (system(cmd) != 0) {
        perror("echo address");
        return 0xFFFFFFFF;
    }

    /* Step 2: Cat config_reg to get value */
    FILE *f = popen("cat " CSU_CONFIG_REG, "r");
    if (!f) {
        perror("popen cat");
        return 0xFFFFFFFF;
    }

    uint32_t val = 0xFFFFFFFF;
    if (fscanf(f, "%x", &val) != 1) val = 0xFFFFFFFF;
    pclose(f);

    return val;
}

int write_csu_reg(uint64_t addr, uint32_t mask, uint32_t value) {
    char cmd[256];

    /* echo <address> <mask> <value> > config_reg */
    snprintf(cmd, sizeof(cmd),
             "echo 0x%lX 0x%X 0x%X > %s",
             addr, mask, value, CSU_CONFIG_REG);

    if (system(cmd) != 0) {
        perror("write_csu_reg");
        return -1;
    }
    return 0;
}

// Read Write Registers Only
int reg_write_check(uint64_t addr, uint32_t mask, uint32_t value){
    write_csu_reg(addr, mask, value);
    return (read_csu_reg(addr) & mask) == (value & mask);

}

int hw_crypto_init(void)
{
    /* Step 0: Reset AES engine first to clear any bad state */
    printf("[0] Resetting AES engine...\n");
    write_csu_reg(AES_RESET, 0x00000001, 0x00000001);  /* assert reset */
    usleep(10000);                                        /* hold reset 10ms */
    write_csu_reg(AES_RESET, 0x00000001, 0x00000000);  /* deassert reset */
    usleep(10000);   

    /* Verify AES came out of reset by checking status is not garbage */
    uint32_t post_reset_status = read_csu_reg(AES_STATUS_ADDR);
    if (post_reset_status == 0xFFFFFFFF) {
        printf("ERROR: AES status unreadable after reset — CSU may not be accessible\n");
        return -1;
    }
    printf("    Post-reset AES STATUS: 0x%08X\n", post_reset_status);

    printf("[1] Setting SSS config...\n");
    if (!reg_write_check(CSU_SSS_CFG_ADDR, 0x00000FF0, 0x000005A0)) {
        printf("ERROR: SSS config failed\n");
        return -1;
    }
    printf("    SSS CFG: 0x%08X\n", read_csu_reg(CSU_SSS_CFG_ADDR));

    printf("[2] Setting key source to KUP...\n");
    if (!reg_write_check(AES_KEY_SRC, 0x0000000F, 0x00000000)) {
        printf("ERROR: Key source set failed\n");
        return -2;
    }
    printf("    KEY_SRC: 0x%08X\n", read_csu_reg(AES_KEY_SRC));

    printf("[3] Writing KUP key...\n");
    if (!aes_key_ready) {
        convert_hex_to_uint32_8(AES_KEY_HEX, AES_KEY);
        aes_key_ready = 1;
    }

    uint64_t kup_addrs[AES_KEY_LEN] = {
        AES_KUP_0_ADDR,  /* bits 255-224 */
        AES_KUP_1_ADDR,  /* bits 223-192 */
        AES_KUP_2_ADDR,  /* bits 191-160 */
        AES_KUP_3_ADDR,  /* bits 159-128 */
        AES_KUP_4_ADDR,  /* bits 127-96  */
        AES_KUP_5_ADDR,  /* bits 95-64   */
        AES_KUP_6_ADDR,  /* bits 63-32   */
        AES_KUP_7_ADDR,  /* bits 31-0    */
    };

    for (int i = 0; i < AES_KEY_LEN; i++) {
        write_csu_reg(kup_addrs[i], 0xFFFFFFFF, AES_KEY[i]);
        printf("    KUP[%d] @ 0x%lX = 0x%08X\n", i, kup_addrs[i], AES_KEY[i]);
    }

    printf("[4] Triggering key load...\n");
    write_csu_reg(AES_KEY_LOAD, 0x00000001, 0x00000001);
    usleep(5000);

    printf("[5] Waiting for key load confirmation...\n");
    int timeout = 1000;
    while (timeout--) {
        uint32_t status = read_csu_reg(AES_STATUS_ADDR);

        if (status == 0xFFFFFFFF) {
            printf("ERROR: Failed to read AES status\n");
            return -3;
        }

        printf("    AES STATUS: 0x%08X | key_init_done=%d | kup_zeroed=%d | busy=%d | ready=%d\n",
               status,
               (status >> 4) & 1,
               (status >> 9) & 1,
               (status >> 0) & 1,
               (status >> 1) & 1);

        if ((status & (1 << 4)) && !(status & (1 << 9))) {
            printf("=== hw_crypto_init SUCCESS: key_init_done confirmed (status=0x%08X) ===\n\n", status);
            return 0;
        }

        if (!(status & (1 << 9))) {
            printf("=== hw_crypto_init SUCCESS: KUP key loaded and held (status=0x%08X) ===\n\n", status);
            return 0;
        }

        usleep(1000);
    }

    printf("ERROR: key never loaded — last status=0x%08X\n", read_csu_reg(AES_STATUS_ADDR));
    return -4;
}