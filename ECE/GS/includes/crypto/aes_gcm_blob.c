#include "aes_gcm_blob.h"
#include <string.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

static int from_hex(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

void hex_encode_lower(const uint8_t *in, size_t len, char *out) {
  static const char *digits = "0123456789abcdef";
  for (size_t i = 0; i < len; i++) {
    out[2*i]     = digits[(in[i] >> 4) & 0xF];
    out[2*i + 1] = digits[in[i] & 0xF];
  }
  out[2*len] = '\0';
}

int hex_decode_bytes(const char *hex, size_t hex_len, uint8_t *out, size_t out_cap, size_t *out_len) {
  if (hex_len % 2 != 0) return -1;
  size_t nbytes = hex_len / 2;
  if (nbytes > out_cap) return -1;

  for (size_t i = 0; i < nbytes; i++) {
    int hi = from_hex(hex[2*i]);
    int lo = from_hex(hex[2*i + 1]);
    if (hi < 0 || lo < 0) return -1;
    out[i] = (uint8_t)((hi << 4) | lo);
  }
  if (out_len) *out_len = nbytes;
  return 0;
}

int aes_gcm_random_nonce(uint8_t nonce[AES_GCM_NONCE_LEN]) {
  // RAND_bytes returns 1 on success
  return (RAND_bytes(nonce, AES_GCM_NONCE_LEN) == 1) ? 0 : -1;
}

int aes_gcm_encrypt_bytes(const uint8_t key[AES_GCM_KEY_LEN],
                          const uint8_t nonce[AES_GCM_NONCE_LEN],
                          const uint8_t *aad, size_t aad_len,
                          const uint8_t *pt, size_t pt_len,
                          uint8_t *ct_out,
                          uint8_t tag_out[AES_GCM_TAG_LEN])
{
  int rc = -1;
  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
  if (!ctx) return -1;

  int ok = EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL);
  ok &= EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, AES_GCM_NONCE_LEN, NULL);
  ok &= EVP_EncryptInit_ex(ctx, NULL, NULL, key, nonce);
  if (!ok) goto cleanup;

  int outl = 0;
  if (aad && aad_len > 0) {
    if (!EVP_EncryptUpdate(ctx, NULL, &outl, aad, (int)aad_len)) goto cleanup;
  }

  int tot = 0;
  if (pt_len > 0) {
    if (!EVP_EncryptUpdate(ctx, ct_out, &outl, pt, (int)pt_len)) goto cleanup;
    tot = outl;
  }

  if (!EVP_EncryptFinal_ex(ctx, ct_out + tot, &outl)) goto cleanup;
  // tot += outl; // usually 0 for GCM

  if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, AES_GCM_TAG_LEN, tag_out)) goto cleanup;

  rc = 0;

cleanup:
  EVP_CIPHER_CTX_free(ctx);
  return rc;
}

int aes_gcm_decrypt_bytes(const uint8_t key[AES_GCM_KEY_LEN],
                          const uint8_t nonce[AES_GCM_NONCE_LEN],
                          const uint8_t tag[AES_GCM_TAG_LEN],
                          const uint8_t *aad, size_t aad_len,
                          const uint8_t *ct, size_t ct_len,
                          uint8_t *pt_out,
                          size_t *pt_len_out)
{
  int rc = -1;
  EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
  if (!ctx) return -1;

  int ok = EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL);
  ok &= EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, AES_GCM_NONCE_LEN, NULL);
  ok &= EVP_DecryptInit_ex(ctx, NULL, NULL, key, nonce);
  if (!ok) goto cleanup;

  int outl = 0;
  if (aad && aad_len > 0) {
    if (!EVP_DecryptUpdate(ctx, NULL, &outl, aad, (int)aad_len)) goto cleanup;
  }

  int tot = 0;
  if (ct_len > 0) {
    if (!EVP_DecryptUpdate(ctx, pt_out, &outl, ct, (int)ct_len)) goto cleanup;
    tot = outl;
  }

  // Set expected tag BEFORE final
  if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, AES_GCM_TAG_LEN, (void*)tag)) goto cleanup;

  if (!EVP_DecryptFinal_ex(ctx, pt_out + tot, &outl)) {
    // auth fail
    rc = -2;
    goto cleanup;
  }
  tot += outl;

  if (pt_len_out) *pt_len_out = (size_t)tot;
  rc = 0;

cleanup:
  EVP_CIPHER_CTX_free(ctx);
  return rc;
}

void aes_gcm_build_blob_hex(const uint8_t nonce[AES_GCM_NONCE_LEN],
                            const uint8_t tag[AES_GCM_TAG_LEN],
                            const uint8_t *ct, size_t ct_len,
                            char *out_hex)
{
  // nonce hex
  hex_encode_lower(nonce, AES_GCM_NONCE_LEN, out_hex);
  // tag hex
  hex_encode_lower(tag, AES_GCM_TAG_LEN, out_hex + 2*AES_GCM_NONCE_LEN);
  // ct hex
  hex_encode_lower(ct, ct_len, out_hex + 2*(AES_GCM_NONCE_LEN + AES_GCM_TAG_LEN));
}

static void strip_hex_compact(const char *in, char *out, size_t out_cap, size_t *out_len) {
  // Copy only hex chars, ignore whitespace
  size_t n = 0;
  for (size_t i = 0; in[i] != '\0'; i++) {
    char c = in[i];
    if (c==' ' || c=='\n' || c=='\r' || c=='\t') continue;
    if (n + 1 >= out_cap) break;
    out[n++] = c;
  }
  out[n] = '\0';
  if (out_len) *out_len = n;
}

int aes_gcm_parse_blob_hex(const char *blob_hex,
                           uint8_t nonce_out[AES_GCM_NONCE_LEN],
                           uint8_t tag_out[AES_GCM_TAG_LEN],
                           uint8_t *ct_out, size_t ct_out_cap,
                           size_t *ct_len_out)
{
  // Need at least nonce+tag: (12+16)*2 = 56 hex chars
  char compact[8192];
  size_t clen = 0;
  strip_hex_compact(blob_hex, compact, sizeof(compact), &clen);

  const size_t min_hex = 2*(AES_GCM_NONCE_LEN + AES_GCM_TAG_LEN);
  if (clen < min_hex) return -1;

  // Nonce
  if (hex_decode_bytes(compact, 2*AES_GCM_NONCE_LEN, nonce_out, AES_GCM_NONCE_LEN, NULL) != 0) return -1;

  // Tag
  const char *tag_hex = compact + 2*AES_GCM_NONCE_LEN;
  if (hex_decode_bytes(tag_hex, 2*AES_GCM_TAG_LEN, tag_out, AES_GCM_TAG_LEN, NULL) != 0) return -1;

  // Ciphertext
  const char *ct_hex = compact + min_hex;
  size_t ct_hex_len = clen - min_hex;
  size_t ct_bytes = 0;
  if (hex_decode_bytes(ct_hex, ct_hex_len, ct_out, ct_out_cap, &ct_bytes) != 0) return -1;

  if (ct_len_out) *ct_len_out = ct_bytes;
  return 0;
}