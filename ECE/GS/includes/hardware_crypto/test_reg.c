#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "hardware_encryption.h"

#define TEST_CIPHERTEXT "8BDF573D3D50AF81FAD29D955B91D6BC09FF99709565F114ACDA91469379B86EC63824B37339EEC7AA903929137BD367D00B0A99534505FB3E603E24D02D77B1DBECA6931B981D18AE316FA14F2B32B8CB778305174D6961FDD3BEA0BE071223C75C3856286788AAEA14BCCD93A3F76FD1DDDD58C824E3467B73FF81F4D4AFAD61DB4398F119542F136E2C6AAB3B68615BCE9D26E148F04226C4994C"

// gcc -O2 -Wall -Wextra test_reg.c hardware_encryption.c -I.../includes/cmd_structure -o test_reg.o

/* Additional defines needed */
#define CSU_DMA_RESET       0x00FFC80008  /* DMA soft reset */
#define CSU_DMA_SRC_CTRL    0x00FFC8000C
#define CSU_DMA_DST_CTRL    0x00FFC80808

uintptr_t virt_to_phys(void *ptr) {
    return (uintptr_t)ptr; // assume virtual == physical
}

int aes_gcm_decrypt(
    uint8_t *iv,
    uint8_t *ciphertext,
    uint32_t ct_len,
    uint8_t *tag,
    uint8_t *plaintext_out
) {
    if ((ct_len % 4) != 0) {
        printf("ERROR: ciphertext length must be multiple of 4\n");
        return -1;
    }

    /* ---------------- [1] Open /dev/mem and map registers ---------------- */
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("open /dev/mem");
        return -2;
    }

    volatile uint32_t *aes_cfg      = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, AES_CFG_ADDR);
    volatile uint32_t *aes_start    = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, AES_START_MSG_ADDR);
    volatile uint32_t *aes_status   = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, AES_STATUS_ADDR);

    volatile uint32_t *dma_src_addr = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, CSU_DMA_SRC_ADDR);
    volatile uint32_t *dma_src_size = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, CSU_DMA_SRC_SIZE);
    volatile uint32_t *dma_src_sts  = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, CSU_DMA_SRC_STS);

    volatile uint32_t *dma_dst_addr = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, CSU_DMA_DST_ADDR);
    volatile uint32_t *dma_dst_size = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, CSU_DMA_DST_SIZE);
    volatile uint32_t *dma_dst_sts  = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, CSU_DMA_DST_STS);

    if (!aes_cfg || !aes_start || !aes_status ||
        !dma_src_addr || !dma_src_size || !dma_src_sts ||
        !dma_dst_addr || !dma_dst_size || !dma_dst_sts) {
        printf("ERROR: mmap failed\n");
        close(fd);
        return -3;
    }

    /* ---------------- [2] Set AES decrypt mode ---------------- */
    reg_write(aes_cfg, 0x1);
    usleep(10);

    /* ---------------- [3] Configure DMA destination ---------------- */
    uint32_t dst_words = ct_len / 4;
    reg_write(dma_dst_addr, (uint32_t)plaintext_out >> 2);
    reg_write(dma_dst_size, dst_words << 2);

    /* ---------------- [4] Start AES engine ---------------- */
    reg_write(aes_start, 0x1);

    /* ---------------- [5] Send IV ---------------- */
    uint32_t iv_words = 16 / 4;
    reg_write(dma_src_addr, (uint32_t)iv >> 2);
    reg_write(dma_src_size, iv_words << 2);  // NOT final

    while (reg_read(dma_src_sts) & (1 << 1)); // wait for DMA SRC idle

    /* ---------------- [6] Send Ciphertext ---------------- */
    uint32_t ct_words = ct_len / 4;
    reg_write(dma_src_addr, (uint32_t)ciphertext >> 2);
    reg_write(dma_src_size, ct_words << 2);  // NOT final

    while (reg_read(dma_src_sts) & (1 << 1));

    /* ---------------- [7] Send GCM tag (final) ---------------- */
    uint32_t tag_words = 16 / 4;
    reg_write(dma_src_addr, (uint32_t)tag >> 2);
    reg_write(dma_src_size, (tag_words << 2) | 0x1); // LAST_WORD=1

    while (reg_read(dma_src_sts) & (1 << 1));

    /* ---------------- [8] Wait for AES DONE ---------------- */
    while (!(reg_read(aes_status) & (1 << 2))) {
        usleep(100);
    }

    int tag_ok = (reg_read(aes_status) >> 3) & 1;
    if (!tag_ok) {
        printf("ERROR: GCM authentication failed\n");
        return -4;
    }

    /* ---------------- [9] Cleanup ---------------- */
    munmap((void *)aes_cfg, MAP_SIZE);
    munmap((void *)aes_start, MAP_SIZE);
    munmap((void *)aes_status, MAP_SIZE);
    munmap((void *)dma_src_addr, MAP_SIZE);
    munmap((void *)dma_src_size, MAP_SIZE);
    munmap((void *)dma_src_sts, MAP_SIZE);
    munmap((void *)dma_dst_addr, MAP_SIZE);
    munmap((void *)dma_dst_size, MAP_SIZE);
    munmap((void *)dma_dst_sts, MAP_SIZE);
    close(fd);

    printf("AES-GCM decryption SUCCESS\n");
    return 0;
}


uint8_t TEST_IV[16]  = {0x00};  // Replace with your actual IV
uint8_t TEST_TAG[16] = {0x00};  // Replace with your actual GCM tag

int main(void)
{
    printf("=== Hardware Crypto Decrypt Test ===\n\n");

    if (hw_crypto_init() != 0) {
        printf("ERROR: hw_crypto_init failed\n");
        return 1;
    }

    /* Allocate buffers */
    uint8_t *ciphertext = calloc(CT_SZ, 1);
    uint8_t *plaintext  = calloc(CT_SZ + 1, 1); // +1 for null-termination
    if (!ciphertext || !plaintext) {
        printf("ERROR: allocation failed\n");
        return 1;
    }

    /* Parse hex string into byte array */
    for (size_t i = 0; i < CT_SZ; i++) {
        sscanf(TEST_CIPHERTEXT + i * 2, "%2hhx", &ciphertext[i]);
    }

    /* Print input for debugging */
    printf("Input ciphertext (%zu bytes):\n    ", CT_SZ);
    for (size_t i = 0; i < CT_SZ; i++) printf("%02X ", ciphertext[i]);
    printf("\n");

    /* Print physical addresses for DMA verification */
    printf("Phys addresses:\n");
    printf("    ciphertext : 0x%lX\n", virt_to_phys(ciphertext));
    printf("    plaintext  : 0x%lX\n", virt_to_phys(plaintext));
    printf("    IV         : 0x%lX\n", virt_to_phys(TEST_IV));
    printf("    TAG        : 0x%lX\n", virt_to_phys(TEST_TAG));

    /* Perform AES-GCM decryption */
    int ret = aes_gcm_decrypt(TEST_IV, ciphertext, CT_SZ, TEST_TAG, plaintext);
    if (ret != 0) {
        printf("ERROR: aes_gcm_decrypt failed with code %d\n", ret);
        free(ciphertext);
        free(plaintext);
        return 1;
    }

    /* Null-terminate for printing as string */
    plaintext[CT_SZ] = '\0';

    printf("Plaintext (%zu bytes):\n    ", CT_SZ);
    for (size_t i = 0; i < CT_SZ; i++) printf("%02X ", plaintext[i]);
    printf("\n\nPlaintext as string:\n    %s\n", plaintext);

    free(ciphertext);
    free(plaintext);
    return 0;
}
