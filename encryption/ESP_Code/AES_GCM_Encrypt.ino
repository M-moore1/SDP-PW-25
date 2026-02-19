/*
 * AES-GCM Encryption for ESP32
 *
 * Reads plaintext from /plaintext.txt on LittleFS,
 * encrypts it, and writes three output files:
 *   /ciphertext.bin  — the encrypted data
 *   /iv.bin          — the random IV used
 *   /tag.bin         — the 16-byte authentication tag
 *
 * The AAD (additional authenticated data) string is configurable below.
 *
 * Tested on: ESP32, ESP32-S2, ESP32-S3, ESP32-C3
 * Requires: LittleFS (built-in with ESP32 Arduino core)
 */

#include "mbedtls/gcm.h"
#include "esp_system.h"
#include "LittleFS.h"

// ── Configuration ────────────────────────────────────────────────────────────
#define AES_KEY_SIZE     32   // 32 bytes = 256-bit key
#define AES_GCM_IV_SIZE  12   // 12 bytes = 96-bit IV (recommended)
#define AES_GCM_TAG_SIZE 16   // 16 bytes = 128-bit authentication tag

// Input / output file paths on LittleFS
#define INPUT_FILE       "/plaintext.txt"
#define OUTPUT_CIPHER    "/ciphertext.bin"
#define OUTPUT_IV        "/iv.bin"
#define OUTPUT_TAG       "/tag.bin"

// Optional Additional Authenticated Data — set to "" to disable
#define AAD_STRING       "esp32_sensor_data"

// AES-256 key — replace with your own secret key.
// NEVER hardcode keys in production; load from secure storage instead.
uint8_t aes_key[AES_KEY_SIZE] = {
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
  0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
};
// ─────────────────────────────────────────────────────────────────────────────

mbedtls_gcm_context gcm_ctx;

// ── Helpers ──────────────────────────────────────────────────────────────────

void printHex(const char* label, const uint8_t* data, size_t len) {
  Serial.print(label);
  Serial.print(": ");
  for (size_t i = 0; i < len; i++) {
    if (data[i] < 0x10) Serial.print("0");
    Serial.print(data[i], HEX);
  }
  Serial.println();
}

void generateRandomIV(uint8_t* iv, size_t ivLen) {
  for (size_t i = 0; i < ivLen; i++) {
    iv[i] = (uint8_t)esp_random();
  }
}

bool writeFile(const char* path, const uint8_t* data, size_t len) {
  File f = LittleFS.open(path, FILE_WRITE);
  if (!f) {
    Serial.printf("ERROR: Could not open %s for writing\n", path);
    return false;
  }
  f.write(data, len);
  f.close();
  Serial.printf("Wrote %d bytes to %s\n", len, path);
  return true;
}

// ── Core encrypt function ────────────────────────────────────────────────────

int aesGcmEncrypt(const uint8_t* plaintext, size_t plaintextLen,
                  const uint8_t* key,  size_t keyBits,
                  const uint8_t* iv,   size_t ivLen,
                  const uint8_t* aad,  size_t aadLen,
                  uint8_t* ciphertext,
                  uint8_t* tag) {
  int ret;

  ret = mbedtls_gcm_setkey(&gcm_ctx, MBEDTLS_CIPHER_ID_AES, key, keyBits);
  if (ret != 0) {
    Serial.printf("ERROR: mbedtls_gcm_setkey failed: -0x%04x\n", -ret);
    return ret;
  }

  ret = mbedtls_gcm_crypt_and_tag(&gcm_ctx,
                                   MBEDTLS_GCM_ENCRYPT,
                                   plaintextLen,
                                   iv, ivLen,
                                   aad, aadLen,
                                   plaintext,
                                   ciphertext,
                                   AES_GCM_TAG_SIZE,
                                   tag);
  if (ret != 0) {
    Serial.printf("ERROR: mbedtls_gcm_crypt_and_tag failed: -0x%04x\n", -ret);
  }
  return ret;
}

// ── Main logic ───────────────────────────────────────────────────────────────

void encryptFile() {
  Serial.println("=== AES-GCM File Encryption ===");
  Serial.println();

  // ── 1. Read plaintext file ────────────────────────────────────────────────
  if (!LittleFS.exists(INPUT_FILE)) {
    Serial.printf("ERROR: Input file '%s' not found.\n", INPUT_FILE);
    Serial.println("Upload it to LittleFS using the Arduino LittleFS upload tool.");
    return;
  }

  File f = LittleFS.open(INPUT_FILE, FILE_READ);
  if (!f) {
    Serial.printf("ERROR: Could not open '%s' for reading.\n", INPUT_FILE);
    return;
  }

  size_t plaintextLen = f.size();
  Serial.printf("Reading '%s' (%d bytes)...\n", INPUT_FILE, plaintextLen);

  uint8_t* plaintext  = (uint8_t*)malloc(plaintextLen);
  uint8_t* ciphertext = (uint8_t*)malloc(plaintextLen);
  if (!plaintext || !ciphertext) {
    Serial.println("ERROR: Memory allocation failed!");
    f.close();
    free(plaintext);
    free(ciphertext);
    return;
  }

  f.read(plaintext, plaintextLen);
  f.close();

  // ── 2. Generate random IV ─────────────────────────────────────────────────
  uint8_t iv[AES_GCM_IV_SIZE];
  generateRandomIV(iv, AES_GCM_IV_SIZE);

  // ── 3. Prepare AAD ────────────────────────────────────────────────────────
  const char* aadStr = AAD_STRING;
  size_t aadLen = strlen(aadStr);

  // ── 4. Encrypt ────────────────────────────────────────────────────────────
  uint8_t tag[AES_GCM_TAG_SIZE];

  Serial.println("Encrypting...");
  unsigned long t0 = micros();

  int ret = aesGcmEncrypt(plaintext, plaintextLen,
                          aes_key, AES_KEY_SIZE * 8,
                          iv, AES_GCM_IV_SIZE,
                          (uint8_t*)aadStr, aadLen,
                          ciphertext, tag);

  unsigned long elapsed = micros() - t0;

  free(plaintext);

  if (ret != 0) {
    Serial.println("ERROR: Encryption failed!");
    free(ciphertext);
    return;
  }

  Serial.printf("Encryption completed in %lu µs\n", elapsed);
  Serial.println();

  // ── 5. Print results to Serial ────────────────────────────────────────────
  printHex("IV        ", iv, AES_GCM_IV_SIZE);
  printHex("Auth Tag  ", tag, AES_GCM_TAG_SIZE);
  printHex("Ciphertext", ciphertext, plaintextLen);
  if (aadLen > 0) {
    Serial.printf("AAD       : %s\n", aadStr);
  }
  Serial.println();

  // ── 6. Write output files ─────────────────────────────────────────────────
  writeFile(OUTPUT_CIPHER, ciphertext, plaintextLen);
  writeFile(OUTPUT_IV,     iv,         AES_GCM_IV_SIZE);
  writeFile(OUTPUT_TAG,    tag,        AES_GCM_TAG_SIZE);

  free(ciphertext);

  Serial.println();
  Serial.println("Done. Transfer " OUTPUT_CIPHER ", " OUTPUT_IV ", and " OUTPUT_TAG);
  Serial.println("to the decryption device.");
}

// ── Arduino entry points ─────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("========================================");
  Serial.println(" AES-GCM File Encryptor  (ESP32)");
  Serial.println("========================================");
  Serial.println();

  if (!LittleFS.begin(true)) {   // 'true' = format if mount fails
    Serial.println("ERROR: LittleFS mount failed!");
    return;
  }
  Serial.println("LittleFS mounted successfully.");
  Serial.println();

  mbedtls_gcm_init(&gcm_ctx);

  encryptFile();

  mbedtls_gcm_free(&gcm_ctx);
}

void loop() {
  // Nothing to do
}
