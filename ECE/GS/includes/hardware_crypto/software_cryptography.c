#include "software_cryptography.h"

#define AES_KEY_HEX "a1b2c3d4e5f6789012345678901234567890abcdef1234567890abcdef123456"

static int g_tfmfd = -1;
static int g_opfd  = -1;

static void convert_key_str(const char *hex, uint8_t *out)
{
    for (int i = 0; i < KEY_SIZE; i++)
        sscanf(hex + i * 2, "%02hhx", &out[i]);
}

static void convert_msg_str(const char *hex, uint8_t *out)
{
    int len = strlen(hex) / 2;
    for (int i = 0; i < len; i++)
        sscanf(hex + i * 2, "%02hhx", &out[i]);
}

int strip_pad(const uint8_t *buf, int len)
{
    while (len > 0 && buf[len - 1] == PAD_BYTE)
        len--;
    return len;
}

int gs_sw_crypto_init()
{
    uint8_t key[KEY_SIZE];

    struct sockaddr_alg sa = {
        .salg_family = AF_ALG,
        .salg_type   = "aead",
        .salg_name   = "gcm(aes)"
    };

    convert_key_str(AES_KEY_HEX, key);

    g_tfmfd = socket(AF_ALG, SOCK_SEQPACKET, 0);
    if (g_tfmfd < 0) return -1;

    if (bind(g_tfmfd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
        close(g_tfmfd); return -2;
    }

    if (setsockopt(g_tfmfd, SOL_ALG, ALG_SET_AEAD_AUTHSIZE, NULL, TAG_SZ) < 0) {
        close(g_tfmfd); return -3;
    }

    if (setsockopt(g_tfmfd, SOL_ALG, ALG_SET_KEY, key, KEY_SIZE) < 0) {
        close(g_tfmfd); return -4;
    }

    g_opfd = accept(g_tfmfd, NULL, 0);
    if (g_opfd < 0) { close(g_tfmfd); return -5; }

    return 0;
}

void gs_sw_crypto_deinit()
{
    if (g_opfd  >= 0) { close(g_opfd);  g_opfd  = -1; }
    if (g_tfmfd >= 0) { close(g_tfmfd); g_tfmfd = -1; }
}

int encrypt_json(const char *Plaintext, uint8_t Ciphertext[TOTAL_SZ])
{
    if (!Plaintext || !Ciphertext) return -1;

    int str_len = strlen(Plaintext);
    if (str_len == 0)    return -2;
    if (str_len > CT_SZ) return -3;     /* max string length is CT_SZ(128) bytes */

    /* Copy string into padded input buffer */
    uint8_t input[CT_SZ];
    memset(input, PAD_BYTE, CT_SZ);
    memcpy(input, Plaintext, str_len);

    /* Generate random IV */
    uint8_t iv[IV_SZ];
    FILE *f = fopen("/dev/urandom", "rb");
    if (!f) return -4;
    if ((int)fread(iv, 1, IV_SZ, f) != IV_SZ) { fclose(f); return -5; }
    fclose(f);

    if (gs_sw_crypto_init() != 0) return -6;

    char cbuf[CMSG_SPACE(4) + CMSG_SPACE(sizeof(struct af_alg_iv) + IV_SZ)];
    memset(cbuf, 0, sizeof(cbuf));

    struct msghdr msg = { .msg_control = cbuf, .msg_controllen = sizeof(cbuf) };

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_ALG;
    cmsg->cmsg_type  = ALG_SET_OP;
    cmsg->cmsg_len   = CMSG_LEN(4);
    *((__u32 *)CMSG_DATA(cmsg)) = ALG_OP_ENCRYPT;

    cmsg = CMSG_NXTHDR(&msg, cmsg);
    cmsg->cmsg_level = SOL_ALG;
    cmsg->cmsg_type  = ALG_SET_IV;
    cmsg->cmsg_len   = CMSG_LEN(sizeof(struct af_alg_iv) + IV_SZ);
    struct af_alg_iv *alg_iv = (struct af_alg_iv *)CMSG_DATA(cmsg);
    alg_iv->ivlen = IV_SZ;
    memcpy(alg_iv->iv, iv, IV_SZ);

    struct iovec iov = { .iov_base = input, .iov_len = CT_SZ };
    msg.msg_iov    = &iov;
    msg.msg_iovlen = 1;

    if (sendmsg(g_opfd, &msg, 0) < 0) { gs_sw_crypto_deinit(); return -7; }

    uint8_t raw_out[CT_SZ + TAG_SZ];
    if (read(g_opfd, raw_out, CT_SZ + TAG_SZ) != CT_SZ + TAG_SZ) {
        gs_sw_crypto_deinit(); return -8;
    }

    gs_sw_crypto_deinit();

    /* Pack raw bytes: IV || ciphertext || tag → TOTAL_SZ(156) bytes */
    memcpy(Ciphertext,          iv,      IV_SZ);
    memcpy(Ciphertext + IV_SZ,  raw_out, CT_SZ + TAG_SZ);

    return 0;
}

int encrypt_cmd(const robot_bt_packet_t *packet, uint8_t *cipher_out, size_t *cipher_out_len)
{
    if (!packet || !cipher_out || !cipher_out_len) return -1;

    uint8_t input[CT_SZ];
    memset(input, PAD_BYTE, CT_SZ);
    memcpy(input, packet->bytes, sizeof(robot_bt_packet_t));

    uint8_t iv[IV_SZ];
    FILE *f = fopen("/dev/urandom", "rb");
    if (!f) return -4;
    if ((int)fread(iv, 1, IV_SZ, f) != IV_SZ) { fclose(f); return -5; }
    fclose(f);

    if (gs_sw_crypto_init() != 0) return -6;

    char cbuf[CMSG_SPACE(4) + CMSG_SPACE(sizeof(struct af_alg_iv) + IV_SZ)];
    memset(cbuf, 0, sizeof(cbuf));

    struct msghdr msg = { .msg_control = cbuf, .msg_controllen = sizeof(cbuf) };

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_ALG;
    cmsg->cmsg_type  = ALG_SET_OP;
    cmsg->cmsg_len   = CMSG_LEN(4);
    *((__u32 *)CMSG_DATA(cmsg)) = ALG_OP_ENCRYPT;

    cmsg = CMSG_NXTHDR(&msg, cmsg);
    cmsg->cmsg_level = SOL_ALG;
    cmsg->cmsg_type  = ALG_SET_IV;
    cmsg->cmsg_len   = CMSG_LEN(sizeof(struct af_alg_iv) + IV_SZ);
    struct af_alg_iv *alg_iv = (struct af_alg_iv *)CMSG_DATA(cmsg);
    alg_iv->ivlen = IV_SZ;
    memcpy(alg_iv->iv, iv, IV_SZ);

    struct iovec iov = { .iov_base = input, .iov_len = CT_SZ };
    msg.msg_iov    = &iov;
    msg.msg_iovlen = 1;

    if (sendmsg(g_opfd, &msg, 0) < 0) { gs_sw_crypto_deinit(); return -7; }

    // Output buffer: IV || ciphertext || tag  (all raw bytes)
    uint8_t raw_out[CT_SZ + TAG_SZ];
    if (read(g_opfd, raw_out, CT_SZ + TAG_SZ) != CT_SZ + TAG_SZ) {
        gs_sw_crypto_deinit(); return -8;
    }

    gs_sw_crypto_deinit();

    // Pack IV || ciphertext || tag into output buffer
    memcpy(cipher_out,          iv,      IV_SZ);
    memcpy(cipher_out + IV_SZ,  raw_out, CT_SZ + TAG_SZ);
    *cipher_out_len = IV_SZ + CT_SZ + TAG_SZ;

    return 0;
}

int decrypt_json(const uint8_t *encrypted, char *json_out, size_t json_out_len)
{
    if (!encrypted || !json_out || json_out_len == 0) return -1;

    _Static_assert(TOTAL_SZ == 156, "TOTAL_SZ must be 156 bytes (IV_SZ + CT_SZ + TAG_SZ)");

    uint8_t output[CT_SZ + 1] = {0};
    struct msghdr  msg  = {0};
    struct cmsghdr *cmsg;
    char cbuf[CMSG_SPACE(4) + CMSG_SPACE(sizeof(struct af_alg_iv) + IV_SZ)] = {0};

    if (gs_sw_crypto_init() != 0) return -2;

    msg.msg_control    = cbuf;
    msg.msg_controllen = sizeof(cbuf);

    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_ALG;
    cmsg->cmsg_type  = ALG_SET_OP;
    cmsg->cmsg_len   = CMSG_LEN(4);
    *(uint32_t *)CMSG_DATA(cmsg) = ALG_OP_DECRYPT;

    cmsg = CMSG_NXTHDR(&msg, cmsg);
    cmsg->cmsg_level = SOL_ALG;
    cmsg->cmsg_type  = ALG_SET_IV;
    cmsg->cmsg_len   = CMSG_LEN(sizeof(struct af_alg_iv) + IV_SZ);
    struct af_alg_iv *iv_ptr = (void *)CMSG_DATA(cmsg);
    iv_ptr->ivlen = IV_SZ;
    memcpy(iv_ptr->iv, encrypted, IV_SZ);

    struct iovec iov = {
        .iov_base = (void *)(encrypted + IV_SZ),
        .iov_len  = CT_SZ + TAG_SZ
    };
    msg.msg_iov    = &iov;
    msg.msg_iovlen = 1;

    if (sendmsg(g_opfd, &msg, 0) < 0) { gs_sw_crypto_deinit(); return -3; }

    int res = read(g_opfd, output, CT_SZ);
    gs_sw_crypto_deinit();

    if (res < 0) return -4;

    /* Strip PAD_BYTE (0xFF) from end of decrypted buffer */
    int str_len = res;
    while (str_len > 0 && output[str_len - 1] == PAD_BYTE)
        str_len--;

    output[str_len] = '\0';

    if ((size_t)str_len >= json_out_len) return -5;
    memcpy(json_out, output, str_len);
    json_out[str_len] = '\0';

    return str_len;
}

int decrypt_cmd(const uint8_t *encrypted, robot_bt_packet_t *pkt_out)
{
    if (!encrypted || !pkt_out) return -1;

    _Static_assert(TOTAL_SZ == 156, "TOTAL_SZ must be 156 bytes (IV_SZ + CT_SZ + TAG_SZ)");

    uint8_t output[CT_SZ] = {0};
    struct msghdr msg = {0};
    struct cmsghdr *cmsg;
    char cbuf[CMSG_SPACE(4) + CMSG_SPACE(sizeof(struct af_alg_iv) + IV_SZ)] = {0};

    if (gs_sw_crypto_init() != 0) return -2;

    msg.msg_control    = cbuf;
    msg.msg_controllen = sizeof(cbuf);

    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_ALG;
    cmsg->cmsg_type  = ALG_SET_OP;
    cmsg->cmsg_len   = CMSG_LEN(4);
    *(uint32_t *)CMSG_DATA(cmsg) = ALG_OP_DECRYPT;

    cmsg = CMSG_NXTHDR(&msg, cmsg);
    cmsg->cmsg_level = SOL_ALG;
    cmsg->cmsg_type  = ALG_SET_IV;
    cmsg->cmsg_len   = CMSG_LEN(sizeof(struct af_alg_iv) + IV_SZ);
    struct af_alg_iv *iv_ptr = (void *)CMSG_DATA(cmsg);
    iv_ptr->ivlen = IV_SZ;
    memcpy(iv_ptr->iv, encrypted, IV_SZ);             /* first IV_SZ(12) bytes */

    struct iovec iov = {
        .iov_base = (void *)(encrypted + IV_SZ),      /* skip IV_SZ(12) prefix */
        .iov_len  = CT_SZ + TAG_SZ                    /* 128 + 16 = 144 bytes  */
    };
    msg.msg_iov    = &iov;
    msg.msg_iovlen = 1;

    if (sendmsg(g_opfd, &msg, 0) < 0) { gs_sw_crypto_deinit(); return -3; }

    int res = read(g_opfd, output, CT_SZ);
    gs_sw_crypto_deinit();

    if (res < 0) return -4;

    memset(pkt_out, 0, sizeof(robot_bt_packet_t));
    memcpy(pkt_out->bytes, output, sizeof(robot_bt_packet_t));

    return 0;
}