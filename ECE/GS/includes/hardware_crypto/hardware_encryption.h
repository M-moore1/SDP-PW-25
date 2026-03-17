#ifndef HARDWARE_ENCRYPTION_H
#define HARDWARE_ENCRYPTION_H

#define CSU_CONFIG_REG "/sys/firmware/zynqmp/config_reg"
//CSU Registers Address
#define AES_STATUS_ADDR     0x00FFCA1000
#define AES_KEY_SRC         0x00FFCA1004
#define AES_KEY_LOAD        0x00FFCA1008
#define AES_START_MSG_ADDR  0x00FFCA100C
#define AES_KEY_CLEAR_ADDR  0x00FFCA1014
#define AES_CFG_ADDR        0x00FFCA1018
#define AES_KUP_7_ADDR      0x00FFCA103C // 0-31
#define AES_KUP_6_ADDR      0x00FFCA1038 // 32-63
#define AES_KUP_5_ADDR      0x00FFCA1034 // 64-95
#define AES_KUP_4_ADDR      0x00FFCA1030 // 96-127
#define AES_KUP_3_ADDR      0x00FFCA102C // 128-159
#define AES_KUP_2_ADDR      0x00FFCA1028 // 160-191
#define AES_KUP_1_ADDR      0x00FFCA1024 // 192-223
#define AES_KUP_0_ADDR      0x00FFCA1020 // 224-255
#define AES_IV_3_ADDR       0x00FFCA104C // Not actually part of IV Extra Data
#define AES_IV_2_ADDR       0x00FFCA1048  
#define AES_IV_1_ADDR       0x00FFCA1044
#define AES_IV_0_ADDR       0x00FFCA1040
#define CSU_SSS_CFG_ADDR    0x00FFCA0008
#define CSU_DMA_SRC_ADDR    0x00FFC80000
#define CSU_DMA_SRC_SIZE    0x00FFC80004
#define CSU_DMA_SRC_STS     0x00FFC80008
#define CSU_DMA_DST_ADDR    0x00FFC80800
#define CSU_DMA_DST_SIZE    0x00FFC80804
#define CSU_DMA_DST_STS     0x00FFC80808



uint32_t read_csu_reg(uint64_t addr);
int write_csu_reg(uint64_t addr, uint32_t mask, uint32_t value);
int gs_cryptography_init();

#endif