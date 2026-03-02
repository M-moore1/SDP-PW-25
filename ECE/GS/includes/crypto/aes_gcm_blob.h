#ifndef AES_GCM_BLOB_H
#define AES_GCM_BLOB_H

#include <stddef.h>
#include <stdint.h>

// Constants for our protocol
#define AES_GCM_KEY_LEN   32
#define AES_GCM_NONCE_LEN 12
#define AES_GCM_TAG_LEN   16

// Encode bytes -> lowercase hex. out must have at least (2*len + 1).
void hex_encode_lower(const uint8_t *in, size_t len, char *out);

// Decode hex -> bytes. Returns 0 on success, -1 on failure.
int hex_decode_bytes(const char *hex, size_t hex_len, uint8_t *out, size_t out_cap, size_t *out_len);

// Generate cryptographically secure random nonce (12 bytes). Returns 0 on success.
int aes_gcm_random_nonce(uint8_t nonce[AES_GCM_NONCE_LEN]);

// Encrypt raw bytes with AES-256-GCM.
// key: 32 bytes, nonce: 12 bytes, aad optional.
// ct_out must have at least pt_len bytes.
// tag_out is 16 bytes.
// Returns 0 on success.
int aes_gcm_encrypt_bytes(const uint8_t key[AES_GCM_KEY_LEN],
                          const uint8_t nonce[AES_GCM_NONCE_LEN],
                          const uint8_t *aad, size_t aad_len,
                          const uint8_t *pt, size_t pt_len,
                          uint8_t *ct_out,
                          uint8_t tag_out[AES_GCM_TAG_LEN]);

// Decrypt raw bytes with AES-256-GCM.
// Returns 0 on success, -2 on auth failure, -1 on other failure.
int aes_gcm_decrypt_bytes(const uint8_t key[AES_GCM_KEY_LEN],
                          const uint8_t nonce[AES_GCM_NONCE_LEN],
                          const uint8_t tag[AES_GCM_TAG_LEN],
                          const uint8_t *aad, size_t aad_len,
                          const uint8_t *ct, size_t ct_len,
                          uint8_t *pt_out,
                          size_t *pt_len_out);

// Build blob_hex = nonce(24 hex) || tag(32 hex) || ct(2*ct_len hex)
// out_hex must have enough room: 2*(12+16+ct_len)+1
void aes_gcm_build_blob_hex(const uint8_t nonce[AES_GCM_NONCE_LEN],
                            const uint8_t tag[AES_GCM_TAG_LEN],
                            const uint8_t *ct, size_t ct_len,
                            char *out_hex);

// Parse blob_hex into nonce/tag/ciphertext bytes.
// blob_hex is a NUL-terminated hex string, optionally with whitespace.
// ct_out must have enough room for ciphertext bytes.
// Returns 0 on success.
int aes_gcm_parse_blob_hex(const char *blob_hex,
                           uint8_t nonce_out[AES_GCM_NONCE_LEN],
                           uint8_t tag_out[AES_GCM_TAG_LEN],
                           uint8_t *ct_out, size_t ct_out_cap,
                           size_t *ct_len_out);

#endif