#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#define CSU_CONFIG_REG "/sys/firmware/zynqmp/config_reg"

// AES Internal Sniffing Registers (Read-only status of what the engine "saw")
#define AES_IV_0         0x00FFCA1040
#define AES_IV_1         0x00FFCA1044
#define AES_IV_2         0x00FFCA1048
#define AES_STATUS       0x00FFCA1000
#define CSU_SSS_CFG      0x00FFCA0008

void write_reg(uint32_t addr, uint32_t val) {
    char cmd[256];
    sprintf(cmd, "echo 0x%08X 0xFFFFFFFF 0x%08X > %s", addr, val, CSU_CONFIG_REG);
    system(cmd);
}

uint32_t read_reg(uint32_t addr) {
    char cmd[256], buf[64];
    sprintf(cmd, "echo 0x%08X > %s", addr, CSU_CONFIG_REG);
    system(cmd);
    FILE *fp = fopen(CSU_CONFIG_REG, "r");
    if (!fp) return 0;
    if (fgets(buf, sizeof(buf), fp) == NULL) { fclose(fp); return 0; }
    fclose(fp);
    return (uint32_t)strtoul(buf, NULL, 16);
}

int main() {
    printf("Pre-check IV_0: 0x%08X\n", read_reg(AES_IV_0));

    // Trigger the flow as we did before
    // Even if memory mapping fails, the CSU DMA will still try to pull from the bus
    // if the addresses we gave it (0x7FE00000) are valid physical RAM.
    
    write_reg(0x00FFC80000, 0x7FE00000); // SRC ADDR
    write_reg(0x00FFC80004, 39);         // SRC SIZE
    write_reg(0x00FFCA100C, 1);          // START MSG

    printf("Waiting for hardware to sniff stream...\n");
    sleep(1);

    uint32_t iv0 = read_reg(AES_IV_0);
    uint32_t status = read_reg(AES_STATUS);

    printf("--- Hardware Observation ---\n");
    printf("AES Status: 0x%08X\n", status);
    printf("Sniffed IV_0: 0x%08X\n", iv0);

    if (iv0 == 0x3D57DF8B) { // First 4 bytes of your IV (little endian)
        printf("SUCCESS: The AES hardware successfully read the IV from RAM!\n");
        printf("This proves the DMA is working, even if we can't see the destination RAM.\n");
    } else {
        printf("FAILURE: IV register is 0x%08X. Hardware did not see the data.\n", iv0);
    }

    return 0;
}