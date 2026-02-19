/*
 * AES-GCM Decryption for ESP32
 *
 * Reads three files from LittleFS (produced by AES_GCM_Encrypt.ino):
 *   /ciphertext.bin  — the encrypted data
 *   /iv.bin          — the 12-byte IV used during encryption
 *   /tag.bin         — the 16-byte authentication tag
 *
 * Decrypts and writes the result to /decrypted.txt.
 * The AAD string must match the one used during encryption.
 *
 * Tested on: ESP32, ESP32-S2, ESP32-S3, ESP32-C3
 * Requires: LittleFS (built-in with ESP32 Arduino core)
 */

#include "mbedtls/gcm.h"
#include "esp_system.h"
#include "LittleFS.h"

// ── Configuration ────────────────────────────────────────────────────────────
#define AES_KEY_SIZE     32   // 32 bytes = 256-bit key
#define AES_GCM_IV_SIZE  12   // 12 bytes = 96-bit IV
#define AES_GCM_TAG_SIZE 16   // 16 bytes = 128-bit authentication tag

// Input file paths (must match what the encryptor wrote)
#define INPUT_CIPHER     "/ciphertext.bin"
#define INPUT_IV         "/iv.bin"
#define INPUT_TAG        "/tag.bin"

// Output file for the recovered plaintext
#define OUTPUT_FILE      "/decrypted.txt"

// Must match the AAD used during encryption — set to "" if none was used
#define AAD_STRING       "esp32_sensor_data"

// AES-256 key — must be identical to the key used for encryption.
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

// Read an entire file into a caller-supplied buffer.
// Returns number of bytes read, or 0 on error.
size_t readFile(const char* path, uint8_t* buf, size_t maxLen) {
  if (!LittleFS.exists(path)) {
    Serial.printf("ERROR: File not found: %s\n", path);
    return 0;
  }
  File f = LittleFS.open(path, FILE_READ);
  if (!f) {
    Serial.printf("ERROR: Could not open %s\n", path);
    return 0;
  }
  size_t len = f.size();
  if (len > maxLen) {
    Serial.printf("ERROR: %s is larger than the supplied buffer (%d > %d)\n",
                  path, len, maxLen);
    f.close();
    return 0;
  }
  f.read(buf, len);
  f.close();
  return len;
}

// ── Core decrypt function ────────────────────────────────────────────────────

int aesGcmDecrypt(const uint8_t* ciphertext, size_t ciphertextLen,
                  const uint8_t* key,  size_t keyBits,
                  const uint8_t* iv,   size_t ivLen,
                  const uint8_t* aad,  size_t aadLen,
                  const uint8_t* tag,
                  uint8_t* plaintext) {
  int ret;

  ret = mbedtls_gcm_setkey(&gcm_ctx, MBEDTLS_CIPHER_ID_AES, key, keyBits);
  if (ret != 0) {
    Serial.printf("ERROR: mbedtls_gcm_setkey failed: -0x%04x\n", -ret);
    return ret;
  }

  ret = mbedtls_gcm_auth_decrypt(&gcm_ctx,
                                  ciphertextLen,
                                  iv, ivLen,
                                  aad, aadLen,
                                  tag, AES_GCM_TAG_SIZE,
                                  ciphertext,
                                  plaintext);
  if (ret != 0) {
    if (ret == MBEDTLS_ERR_GCM_AUTH_FAILED) {
      Serial.println("ERROR: Authentication FAILED — data may have been tampered with!");
    } else {
      Serial.printf("ERROR: mbedtls_gcm_auth_decrypt failed: -0x%04x\n", -ret);
    }
  }
  return ret;
}

// ── Main logic ───────────────────────────────────────────────────────────────

void decryptFile() {
  Serial.println("=== AES-GCM File Decryption ===");
  Serial.println();

  // ── 1. Read fixed-size IV and tag ─────────────────────────────────────────
  uint8_t iv[AES_GCM_IV_SIZE];
  uint8_t tag[AES_GCM_TAG_SIZE];

  if (readFile(INPUT_IV,  iv,  AES_GCM_IV_SIZE)  != AES_GCM_IV_SIZE)  return;
  if (readFile(INPUT_TAG, tag, AES_GCM_TAG_SIZE) != AES_GCM_TAG_SIZE) return;

  // ── 2. Read ciphertext ────────────────────────────────────────────────────
  if (!LittleFS.exists(INPUT_CIPHER)) {
    Serial.printf("ERROR: Ciphertext file '%s' not found.\n", INPUT_CIPHER);
    return;
  }
  size_t ciphertextLen = LittleFS.open(INPUT_CIPHER, FILE_READ).size();

  uint8_t* ciphertext = (uint8_t*)malloc(ciphertextLen);
  uint8_t* plaintext  = (uint8_t*)malloc(ciphertextLen + 1); // +1 for null terminator
  if (!ciphertext || !plaintext) {
    Serial.println("ERROR: Memory allocation failed!");
    free(ciphertext);
    free(plaintext);
    return;
  }

  if (readFile(INPUT_CIPHER, ciphertext, ciphertextLen) != ciphertextLen) {
    free(ciphertext);
    free(plaintext);
    return;
  }

  Serial.printf("Ciphertext: %d bytes read from %s\n", ciphertextLen, INPUT_CIPHER);

  // ── 3. Prepare AAD ────────────────────────────────────────────────────────
  const char* aadStr = AAD_STRING;
  size_t aadLen = strlen(aadStr);

  // ── 4. Print inputs ───────────────────────────────────────────────────────
  printHex("IV       ", iv, AES_GCM_IV_SIZE);
  printHex("Auth Tag ", tag, AES_GCM_TAG_SIZE);
  if (aadLen > 0) {
    Serial.printf("AAD      : %s\n", aadStr);
  }
  Serial.println();

  // ── 5. Decrypt ────────────────────────────────────────────────────────────
  Serial.println("Decrypting...");
  unsigned long t0 = micros();

  int ret = aesGcmDecrypt(ciphertext, ciphertextLen,
                          aes_key, AES_KEY_SIZE * 8,
                          iv, AES_GCM_IV_SIZE,
                          (uint8_t*)aadStr, aadLen,
                          tag,
                          plaintext);

  unsigned long elapsed = micros() - t0;
  free(ciphertext);

  if (ret != 0) {
    Serial.println("Decryption FAILED. Output file not written.");
    free(plaintext);
    return;
  }

  Serial.printf("Decryption completed in %lu µs\n", elapsed);
  Serial.println("Authentication tag verified ✓");
  Serial.println();

  // ── 6. Print and save plaintext ───────────────────────────────────────────
  plaintext[ciphertextLen] = '\0';  // safe null terminator for text display
  Serial.print("Decrypted text: ");
  Serial.println((char*)plaintext);
  Serial.println();

  File out = LittleFS.open(OUTPUT_FILE, FILE_WRITE);
  if (!out) {
    Serial.printf("ERROR: Could not open '%s' for writing.\n", OUTPUT_FILE);
  } else {
    out.write(plaintext, ciphertextLen);
    out.close();
    Serial.printf("Plaintext written to '%s' (%d bytes).\n", OUTPUT_FILE, ciphertextLen);
  }

  free(plaintext);
}

// ── Arduino entry points ─────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("========================================");
  Serial.println(" AES-GCM File Decryptor  (ESP32)");
  Serial.println("========================================");
  Serial.println();

  if (!LittleFS.begin(true)) {
    Serial.println("ERROR: LittleFS mount failed!");
    return;
  }
  Serial.println("LittleFS mounted successfully.");
  Serial.println();

  mbedtls_gcm_init(&gcm_ctx);

  decryptFile();

  mbedtls_gcm_free(&gcm_ctx);
}

void loop() {
  // Nothing to do
}
