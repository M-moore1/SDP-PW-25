/*
 * AES-GCM Encryption for ESP32
 * 
 * This sketch demonstrates AES-GCM authenticated encryption on ESP32
 * using the built-in mbedTLS library (hardware accelerated on ESP32).
 * 
 * Tested on: ESP32, ESP32-S2, ESP32-S3, ESP32-C3
 * 
 * No additional libraries required - uses built-in mbedTLS!
 */

/*
921600
/dev/tty.usbserial-1110	
*/





#include "mbedtls/gcm.h"
#include "esp_system.h"

// Configuration
#define AES_KEY_SIZE 32        // 32 bytes = 256-bit key
#define AES_GCM_IV_SIZE 12     // 12 bytes = 96-bit IV (recommended)
#define AES_GCM_TAG_SIZE 16    // 16 bytes = 128-bit authentication tag

// Global GCM context
mbedtls_gcm_context gcm_ctx;

// Test key (NEVER use hardcoded keys in production!)
uint8_t test_key[AES_KEY_SIZE] = {
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
  0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
};

// Test IV (must be unique for each encryption with same key)
uint8_t test_iv[AES_GCM_IV_SIZE] = {
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
  0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b
};

void setup() {
  
  Serial.begin(115200);
  delay(1000); // Give serial time to initialize
  

  pinMode(17, OUTPUT);


  Serial.println();
  Serial.println("========================================");
  Serial.println("AES-GCM Encryption for ESP32");
  Serial.println("Using Hardware-Accelerated mbedTLS");
  Serial.println("========================================");
  Serial.println();
  
  // Initialize mbedTLS GCM context
  mbedtls_gcm_init(&gcm_ctx);
  

  
  // Test authentication failure detection
  Serial.println();
  testAuthenticationFailure();
  
  // Benchmark performance
  Serial.println();
  benchmarkPerformance();

}

void loop() {
  // Nothing to do in loop
  // Run encryption/decryption demo
  runEncryptionDemo();
  delay(1000);
}

/**
 * Encrypt data using AES-GCM
 * 
 * @param plaintext     Input plaintext data
 * @param plaintextLen  Length of plaintext
 * @param key           AES key (16, 24, or 32 bytes)
 * @param keyLen        Length of key in bits (128, 192, or 256)
 * @param iv            Initialization vector (12 bytes recommended)
 * @param ivLen         Length of IV
 * @param aad           Additional authenticated data (optional)
 * @param aadLen        Length of AAD
 * @param ciphertext    Output buffer for ciphertext
 * @param tag           Output authentication tag (16 bytes)
 * 
 * @return 0 on success, negative on error
 */
int aesGcmEncrypt(const uint8_t* plaintext, size_t plaintextLen,
                  const uint8_t* key, size_t keyLen,
                  const uint8_t* iv, size_t ivLen,
                  const uint8_t* aad, size_t aadLen,
                  uint8_t* ciphertext,
                  uint8_t* tag) {
  
  int ret;
  
  // Set the key (keyLen is in bits: 128, 192, or 256)
  ret = mbedtls_gcm_setkey(&gcm_ctx, MBEDTLS_CIPHER_ID_AES, key, keyLen);
  if (ret != 0) {
    Serial.printf("Error: mbedtls_gcm_setkey failed: -0x%04x\n", -ret);
    return ret;
  }
  
  // Encrypt and authenticate
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
    Serial.printf("Error: mbedtls_gcm_crypt_and_tag failed: -0x%04x\n", -ret);
    return ret;
  }
  
  return 0;
}

/**
 * Decrypt and verify data using AES-GCM
 * 
 * @param ciphertext    Input ciphertext data
 * @param ciphertextLen Length of ciphertext
 * @param key           AES key (same as encryption)
 * @param keyLen        Length of key in bits (128, 192, or 256)
 * @param iv            Initialization vector (same as encryption)
 * @param ivLen         Length of IV
 * @param aad           Additional authenticated data (same as encryption)
 * @param aadLen        Length of AAD
 * @param tag           Authentication tag from encryption
 * @param plaintext     Output buffer for plaintext
 * 
 * @return 0 on success, negative on error (including authentication failure)
 */
int aesGcmDecrypt(const uint8_t* ciphertext, size_t ciphertextLen,
                  const uint8_t* key, size_t keyLen,
                  const uint8_t* iv, size_t ivLen,
                  const uint8_t* aad, size_t aadLen,
                  const uint8_t* tag,
                  uint8_t* plaintext) {
  
  int ret;
  
  // Set the key (keyLen is in bits: 128, 192, or 256)
  ret = mbedtls_gcm_setkey(&gcm_ctx, MBEDTLS_CIPHER_ID_AES, key, keyLen);
  if (ret != 0) {
    Serial.printf("Error: mbedtls_gcm_setkey failed: -0x%04x\n", -ret);
    return ret;
  }
  
  // Decrypt and verify
  ret = mbedtls_gcm_auth_decrypt(&gcm_ctx,
                                  ciphertextLen,
                                  iv, ivLen,
                                  aad, aadLen,
                                  tag, AES_GCM_TAG_SIZE,
                                  ciphertext,
                                  plaintext);
  
  if (ret != 0) {
    if (ret == MBEDTLS_ERR_GCM_AUTH_FAILED) {
      Serial.println("Error: Authentication failed - data was tampered!");
    } else {
      Serial.printf("Error: mbedtls_gcm_auth_decrypt failed: -0x%04x\n", -ret);
    }
    return ret;
  }
  
  return 0;
}

/**
 * Generate a random IV using ESP32's hardware RNG
 */
void generateRandomIV(uint8_t* iv, size_t ivLen) {
  for (size_t i = 0; i < ivLen; i++) {
    iv[i] = (uint8_t)esp_random();
  }
}

/**
 * Print hex data
 */
void printHex(const char* label, const uint8_t* data, size_t len) {
  Serial.print(label);
  Serial.print(": ");
  for (size_t i = 0; i < len; i++) {
    if (data[i] < 0x10) Serial.print("0");
    Serial.print(data[i], HEX);
  }
  Serial.println();
}

/**
 * Run encryption and decryption demonstration
 */
void runEncryptionDemo() {
  Serial.println("=== Encryption Demo ===");
  Serial.println();
  
  // Message to encrypt
  const char* message = "Hello ESP32! This is a secret message using AES-GCM.";
  size_t messageLen = strlen(message);
  
  // Additional authenticated data (metadata)
  const char* aad = "sensor_data";
  size_t aadLen = strlen(aad);
  
  // Allocate buffers
  uint8_t* ciphertext = (uint8_t*)malloc(messageLen);
  uint8_t tag[AES_GCM_TAG_SIZE];
  uint8_t* decrypted = (uint8_t*)malloc(messageLen + 1);
  
  if (!ciphertext || !decrypted) {
    Serial.println("Error: Memory allocation failed!");
    return;
  }
  
  // Print original message
  Serial.print("Plaintext: ");
  Serial.println(message);
  Serial.printf("Length: %d bytes\n", messageLen);
  printHex("Key", test_key, AES_KEY_SIZE);
  printHex("IV", test_iv, AES_GCM_IV_SIZE);
  Serial.print("AAD: ");
  Serial.println(aad);
  Serial.println();
  
  // Encrypt
  Serial.println("Encrypting...");
  unsigned long startTime = micros();
  
  int ret = aesGcmEncrypt((uint8_t*)message, messageLen,
                          test_key, AES_KEY_SIZE * 8,  // Key length in bits
                          test_iv, AES_GCM_IV_SIZE,
                          (uint8_t*)aad, aadLen,
                          ciphertext, tag);
  
  unsigned long encryptTime = micros() - startTime;
  
  if (ret != 0) {
    Serial.println("Encryption failed!");
    free(ciphertext);
    free(decrypted);
    return;
  }
  
  Serial.printf("Encryption took: %lu microseconds\n", encryptTime);
  printHex("Ciphertext", ciphertext, messageLen);
  printHex("Auth Tag", tag, AES_GCM_TAG_SIZE);
  Serial.println();
  
  // Decrypt
  Serial.println("Decrypting...");
  startTime = micros();
  
  ret = aesGcmDecrypt(ciphertext, messageLen,
                      test_key, AES_KEY_SIZE * 8,  // Key length in bits
                      test_iv, AES_GCM_IV_SIZE,
                      (uint8_t*)aad, aadLen,
                      tag, decrypted);
  
  unsigned long decryptTime = micros() - startTime;
  
  if (ret != 0) {
    Serial.println("Decryption failed!");
    free(ciphertext);
    free(decrypted);
    return;
  }
  
  Serial.printf("Decryption took: %lu microseconds\n", decryptTime);
  
  // Null-terminate and print decrypted text
  decrypted[messageLen] = '\0';
  Serial.print("Decrypted: ");
  Serial.println((char*)decrypted);
  Serial.println();
  
  // Verify decryption matches original
  if (memcmp(message, decrypted, messageLen) == 0) {
    Serial.println("✓ SUCCESS: Decrypted text matches original!");
  } else {
    Serial.println("✗ FAILURE: Decrypted text does not match!");
  }
  
  // Clean up
  free(ciphertext);
  free(decrypted);


  digitalWrite(17, HIGH);
  delay(1000);
  digitalWrite(17, LOW);
  delay(1000);

}

/**
 * Test authentication failure detection
 */
void testAuthenticationFailure() {
  Serial.println("=== Testing Authentication Failure ===");
  Serial.println();
  
  const char* message = "Test message";
  size_t messageLen = strlen(message);
  
  uint8_t ciphertext[64];
  uint8_t tag[AES_GCM_TAG_SIZE];
  uint8_t decrypted[64];
  
  // Encrypt
  Serial.println("Encrypting test message...");
  int ret = aesGcmEncrypt((uint8_t*)message, messageLen,
                          test_key, AES_KEY_SIZE * 8,
                          test_iv, AES_GCM_IV_SIZE,
                          NULL, 0,
                          ciphertext, tag);
  
  if (ret != 0) {
    Serial.println("Encryption failed!");
    return;
  }
  
  Serial.println("✓ Encryption successful");
  Serial.println();
  
  // Tamper with ciphertext
  Serial.println("Tampering with ciphertext (flipping one bit)...");
  ciphertext[0] ^= 0x01;
  
  // Try to decrypt tampered data
  ret = aesGcmDecrypt(ciphertext, messageLen,
                      test_key, AES_KEY_SIZE * 8,
                      test_iv, AES_GCM_IV_SIZE,
                      NULL, 0,
                      tag, decrypted);
  
  if (ret != 0) {
    Serial.println("✓ SUCCESS: Authentication correctly detected tampering!");
  } else {
    Serial.println("✗ FAILURE: Tampering was not detected!");
  }
}

/**
 * Benchmark encryption/decryption performance
 */
void benchmarkPerformance() {
  Serial.println("=== Performance Benchmark ===");
  Serial.println();
  
  const size_t testSizes[] = {16, 64, 256, 1024, 4096};
  const int iterations = 100;
  
  Serial.println("Testing encryption/decryption speed:");
  Serial.println("Size (bytes) | Encrypt (μs) | Decrypt (μs) | Throughput (KB/s)");
  Serial.println("-------------|--------------|--------------|------------------");
  
  for (size_t i = 0; i < sizeof(testSizes) / sizeof(testSizes[0]); i++) {
    size_t dataSize = testSizes[i];
    
    uint8_t* plaintext = (uint8_t*)malloc(dataSize);
    uint8_t* ciphertext = (uint8_t*)malloc(dataSize);
    uint8_t* decrypted = (uint8_t*)malloc(dataSize);
    uint8_t tag[AES_GCM_TAG_SIZE];
    uint8_t iv[AES_GCM_IV_SIZE];
    
    if (!plaintext || !ciphertext || !decrypted) {
      Serial.println("Memory allocation failed!");
      free(plaintext);
      free(ciphertext);
      free(decrypted);
      continue;
    }
    
    // Fill with random data
    for (size_t j = 0; j < dataSize; j++) {
      plaintext[j] = (uint8_t)esp_random();
    }
    generateRandomIV(iv, AES_GCM_IV_SIZE);
    
    // Benchmark encryption
    unsigned long encryptTotal = 0;
    for (int j = 0; j < iterations; j++) {
      unsigned long start = micros();
      aesGcmEncrypt(plaintext, dataSize,
                    test_key, AES_KEY_SIZE * 8,
                    iv, AES_GCM_IV_SIZE,
                    NULL, 0,
                    ciphertext, tag);
      encryptTotal += (micros() - start);
    }
    unsigned long avgEncrypt = encryptTotal / iterations;
    
    // Benchmark decryption
    unsigned long decryptTotal = 0;
    for (int j = 0; j < iterations; j++) {
      unsigned long start = micros();
      aesGcmDecrypt(ciphertext, dataSize,
                    test_key, AES_KEY_SIZE * 8,
                    iv, AES_GCM_IV_SIZE,
                    NULL, 0,
                    tag, decrypted);
      decryptTotal += (micros() - start);
    }
    unsigned long avgDecrypt = decryptTotal / iterations;
    
    // Calculate throughput (KB/s)
    float throughput = (dataSize / 1024.0) / (avgEncrypt / 1000000.0);
    
    Serial.printf("%12d | %12lu | %12lu | %16.2f\n",
                  dataSize, avgEncrypt, avgDecrypt, throughput);
    
    free(plaintext);
    free(ciphertext);
    free(decrypted);
  }
  
  Serial.println();
  Serial.printf("Free heap after benchmark: %d bytes\n", esp_get_free_heap_size());


}
