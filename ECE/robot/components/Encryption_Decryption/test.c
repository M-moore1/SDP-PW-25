#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "mbedtls/gcm.h"

/* ================= CONFIG ================= */

#define AES_KEY_SIZE       32      // 256-bit
#define AES_GCM_IV_SIZE    12
#define AES_GCM_TAG_SIZE   16

#define LED_PIN GPIO_NUM_17

static const char *TAG = "AES_GCM";

/* ================= GLOBALS ================= */

static mbedtls_gcm_context gcm_ctx;

static uint8_t test_key[AES_KEY_SIZE] = {
    0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
    0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
    0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
    0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F
};

static uint8_t test_iv[AES_GCM_IV_SIZE] = {
    0x00,0x01,0x02,0x03,0x04,0x05,
    0x06,0x07,0x08,0x09,0x0A,0x0B
};

/* ================= GPIO ================= */

static void led_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_conf);
}

/* ================= UTILS ================= */

static void printHex(const char *label, const uint8_t *data, size_t len) {
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02X", data[i]);
    }
    printf("\n");
}

/* ================= AES-GCM ================= */

static int aesGcmEncrypt(const uint8_t *plaintext, size_t plaintextLen,
                         const uint8_t *key, size_t keyBits,
                         const uint8_t *iv, size_t ivLen,
                         const uint8_t *aad, size_t aadLen,
                         uint8_t *ciphertext,
                         uint8_t *tag) {

    int ret = mbedtls_gcm_setkey(&gcm_ctx, MBEDTLS_CIPHER_ID_AES, key, keyBits);
    if (ret != 0) return ret;

    return mbedtls_gcm_crypt_and_tag(
        &gcm_ctx,
        MBEDTLS_GCM_ENCRYPT,
        plaintextLen,
        iv, ivLen,
        aad, aadLen,
        plaintext,
        ciphertext,
        AES_GCM_TAG_SIZE,
        tag
    );
}

static int aesGcmDecrypt(const uint8_t *ciphertext, size_t ciphertextLen,
                         const uint8_t *key, size_t keyBits,
                         const uint8_t *iv, size_t ivLen,
                         const uint8_t *aad, size_t aadLen,
                         const uint8_t *tag,
                         uint8_t *plaintext) {

    int ret = mbedtls_gcm_setkey(&gcm_ctx, MBEDTLS_CIPHER_ID_AES, key, keyBits);
    if (ret != 0) return ret;

    return mbedtls_gcm_auth_decrypt(
        &gcm_ctx,
        ciphertextLen,
        iv, ivLen,
        aad, aadLen,
        tag, AES_GCM_TAG_SIZE,
        ciphertext,
        plaintext
    );
}

/* ================= DEMOS ================= */

static void runEncryptionDemo(void) {
    ESP_LOGI(TAG, "=== Encryption Demo ===");

    const char *message = "Hello ESP32! AES-GCM via ESP-IDF.";
    const char *aad = "sensor_data";

    size_t msgLen = strlen(message);

    uint8_t *ciphertext = malloc(msgLen);
    uint8_t *decrypted  = malloc(msgLen + 1);
    uint8_t tag[AES_GCM_TAG_SIZE];

    int64_t start = esp_timer_get_time();

    aesGcmEncrypt((uint8_t *)message, msgLen,
                  test_key, AES_KEY_SIZE * 8,
                  test_iv, AES_GCM_IV_SIZE,
                  (uint8_t *)aad, strlen(aad),
                  ciphertext, tag);

    int64_t encTime = esp_timer_get_time() - start;

    start = esp_timer_get_time();

    int ret = aesGcmDecrypt(ciphertext, msgLen,
                            test_key, AES_KEY_SIZE * 8,
                            test_iv, AES_GCM_IV_SIZE,
                            (uint8_t *)aad, strlen(aad),
                            tag, decrypted);

    int64_t decTime = esp_timer_get_time() - start;

    decrypted[msgLen] = 0;

    printHex("Key", test_key, AES_KEY_SIZE);
    printHex("IV", test_iv, AES_GCM_IV_SIZE);
    printHex("Ciphertext", ciphertext, msgLen);
    printHex("Auth Tag", tag, AES_GCM_TAG_SIZE);

    ESP_LOGI(TAG, "Encrypt time: %lld us", encTime);
    ESP_LOGI(TAG, "Decrypt time: %lld us", decTime);

    if (ret == 0 && strcmp(message, (char *)decrypted) == 0) {
        ESP_LOGI(TAG, "✓ Decryption successful");
    } else {
        ESP_LOGE(TAG, "✗ Decryption failed");
    }

    gpio_set_level(LED_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(500));
    gpio_set_level(LED_PIN, 0);

    free(ciphertext);
    free(decrypted);
}

static void testAuthenticationFailure(void) {
    ESP_LOGI(TAG, "=== Authentication Failure Test ===");

    const char *msg = "Test message";

    uint8_t ciphertext[64];
    uint8_t decrypted[64];
    uint8_t tag[AES_GCM_TAG_SIZE];

    aesGcmEncrypt((uint8_t *)msg, strlen(msg),
                  test_key, AES_KEY_SIZE * 8,
                  test_iv, AES_GCM_IV_SIZE,
                  NULL, 0,
                  ciphertext, tag);

    ciphertext[0] ^= 0x01;

    int ret = aesGcmDecrypt(ciphertext, strlen(msg),
                            test_key, AES_KEY_SIZE * 8,
                            test_iv, AES_GCM_IV_SIZE,
                            NULL, 0,
                            tag, decrypted);

    if (ret == MBEDTLS_ERR_GCM_AUTH_FAILED) {
        ESP_LOGI(TAG, "✓ Tampering detected correctly");
    } else {
        ESP_LOGE(TAG, "✗ Tampering NOT detected");
    }
}

/* ================= ENTRY ================= */
/*
void app_main(void) {
    led_init();
    mbedtls_gcm_init(&gcm_ctx);

    ESP_LOGI(TAG, "AES-GCM ESP-IDF Demo Started");
    ESP_LOGI(TAG, "Free heap: %lu bytes", esp_get_free_heap_size());

    testAuthenticationFailure();

    while (1) {
        runEncryptionDemo();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
*/