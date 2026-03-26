#ifndef AES_GCM_ENCRYPT_H
#define AES_GCM_ENCRYPT_H

#include <stdint.h>
#include <stdlib.h>

/**
 * Encrypt a plaintext buffer into a 156-byte AES-256-GCM packet.
 *
 * Packet layout (all raw bytes, no hex encoding):
 *   [0  .. 11 ]  nonce      (12 bytes)  — randomly generated per call
 *   [12 .. 139]  ciphertext (128 bytes)
 *   [140.. 155]  GCM tag    (16 bytes)
 *
 * The plaintext must be exactly 128 bytes.  If your message is shorter,
 * pad it before calling this function (e.g. with zeros or PKCS-style
 * padding), and strip the padding after decryption.
 *
 * @param plaintext        128-byte input buffer to encrypt.
 * @param out_packet       Caller-allocated 156-byte output buffer that will
 *                         receive [nonce | ciphertext | tag].
 *
 * @return  0  on success
 *         -1  on argument / initialisation error
 *         -3  on nonce generation failure
 */
int aes_gcm_encrypt_packet(const char    *plaintext,
                           uint8_t        out_packet[156]);

#endif /* AES_GCM_ENCRYPT_H */
