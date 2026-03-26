#ifndef AES_GCM_DECRYPT_H
#define AES_GCM_DECRYPT_H

#include <stdint.h>
#include <stdlib.h>

/**
 * Decrypt a 156-byte AES-256-GCM packet.
 *
 * Packet layout (all raw bytes, no hex encoding):
 *   [0  .. 11 ]  nonce      (12 bytes)
 *   [12 .. 139]  ciphertext (128 bytes)
 *   [140.. 155]  GCM tag    (16 bytes)
 *
 * @param received_packet  Raw 156-byte input buffer.
 * @param out_plaintext    Caller-allocated buffer for decrypted output.
 *                         Must be at least 129 bytes (128 CT bytes + NUL).
 * @param out_len          Set to the number of plaintext bytes written
 *                         (not counting the NUL terminator).
 *
 * @return  0  on success (tag verified, plaintext written + NUL-terminated)
 *         -1  on argument / memory error
 *         -2  on authentication failure (bad key / corrupted packet)
 */
int aes_gcm_decrypt_packet(const uint8_t received_packet[156],
                           char         *out_plaintext,
                           size_t       *out_len);

#endif /* AES_GCM_DECRYPT_H */