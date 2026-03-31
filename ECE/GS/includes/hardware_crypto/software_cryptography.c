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

int sw_encryption(const char *Plaintext, char Ciphertext[PAYLOAD_HEX_STR_LEN + 1])
{
    /* 1. Decode hex → raw bytes, then 0xFF-pad to CT_SZ */
    int hex_len  = strlen(Plaintext);
    if (hex_len % 2 != 0) return -1;
    int data_len = hex_len / 2;
    if (data_len > CT_SZ) return -2;

    uint8_t input[CT_SZ];
    memset(input, PAD_BYTE, CT_SZ);               
    for (int i = 0; i < data_len; i++)
        if (sscanf(Plaintext + i * 2, "%02hhx", &input[i]) != 1) return -3;

    /* 2. Random IV */
    uint8_t iv[IV_SZ];
    FILE *f = fopen("/dev/urandom", "rb");
    if (!f) return -4;
    if ((int)fread(iv, 1, IV_SZ, f) != IV_SZ) { fclose(f); return -5; }
    fclose(f);

    /* 3. Init */
    if (gs_sw_crypto_init() != 0) return -6;

    /* 4. Control message: op=ENCRYPT + IV */
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

    /* 5. Send full CT_SZ padded plaintext */
    struct iovec iov = { .iov_base = input, .iov_len = CT_SZ };
    msg.msg_iov    = &iov;
    msg.msg_iovlen = 1;

    if (sendmsg(g_opfd, &msg, 0) < 0) { gs_sw_crypto_deinit(); return -7; }

    /* 6. Read CT_SZ ciphertext bytes + TAG_SZ tag bytes */
    uint8_t raw_out[CT_SZ + TAG_SZ];
    if (read(g_opfd, raw_out, CT_SZ + TAG_SZ) != CT_SZ + TAG_SZ) {
        gs_sw_crypto_deinit(); return -8;
    }

    gs_sw_crypto_deinit();

    // 7. Hex-encode: IV || ciphertext || tag → 312 hex chars 
    int pos = 0;
    for (int i = 0; i < IV_SZ;          i++) pos += sprintf(Ciphertext + pos, "%02x", iv[i]);
    for (int i = 0; i < CT_SZ + TAG_SZ; i++) pos += sprintf(Ciphertext + pos, "%02x", raw_out[i]);
    Ciphertext[pos] = '\0';

    return 0;
}

int sw_decryption(const char *Ciphertext, uint8_t output[CT_SZ + 1])
{
    uint8_t msg_raw[TOTAL_SZ];
    struct msghdr msg = {0};
    struct cmsghdr *cmsg;
    char cbuf[CMSG_SPACE(4) + CMSG_SPACE(sizeof(struct af_alg_iv) + IV_SZ)] = {0};

    if (gs_sw_crypto_init() != 0) return -1;

    convert_msg_str(Ciphertext, msg_raw);

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
    memcpy(iv_ptr->iv, msg_raw, IV_SZ);           /* IV is first 12 bytes */

    struct iovec iov = {
        .iov_base = msg_raw + IV_SZ,              /* skip IV prefix        */
        .iov_len  = CT_SZ + TAG_SZ                /* 128 + 16 = 144 bytes  */
    };
    msg.msg_iov    = &iov;
    msg.msg_iovlen = 1;

    if (sendmsg(g_opfd, &msg, 0) < 0) { gs_sw_crypto_deinit(); return -2; }

    int res = read(g_opfd, output, CT_SZ);
    gs_sw_crypto_deinit();

    if (res < 0) return -3;

    output[res] = '\0';
    return res;
}

int encrypt_instr(const uint8_t instruction[8], char encrypted_string[PAYLOAD_HEX_STR_LEN+1])
{
    if (!instruction || !encrypted_string) return -1;

    char input_hex[17];
    for (int i = 0; i < 8; i++)
        sprintf(input_hex + i * 2, "%02x", instruction[i]);
    input_hex[16] = '\0';

    if (sw_encryption(input_hex, encrypted_string) != 0) return -2;

    return 0;
}