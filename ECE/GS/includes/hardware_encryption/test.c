#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>

#define CSU_CONFIG_REG "/sys/firmware/zynqmp/config_reg"
#define AES_CFG_ADDR 0x00FFCA1018

// Read register using echo+cat method (write address, then read)
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

// Write register using ASCII string
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

int main() {
    printf("Reading AES_CFG register...\n");
    uint32_t val = read_csu_reg(AES_CFG_ADDR);
    printf("AES_CFG initial = 0x%08X\n", val);

    printf("Writing 0x0 to AES_CFG register (set Decryption)...\n");
    if(write_csu_reg(AES_CFG_ADDR, 0xFFFFFFFF, 0x01) != 0) {
        printf("Failed to write AES_CFG\n");
        return 1;
    }

    printf("Reading AES_CFG register again...\n");
    val = read_csu_reg(AES_CFG_ADDR);
    printf("AES_CFG after write = 0x%08X\n", val);

    if (val & 0x1)
        printf("AES Mode: Encryption\n");
    else
        printf("AES Mode: Decryption\n");

    return 0;
}