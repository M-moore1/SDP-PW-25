// aes_gcm_decrypt_split.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>

#define NONCE_HEX_LEN 24   // 12 bytes
#define TAG_HEX_LEN   32   // 16 bytes

// ---------- helpers ----------
static int from_hex(char c){
    if(c>='0'&&c<='9')return c-'0';
    if(c>='a'&&c<='f')return c-'a'+10;
    if(c>='A'&&c<='F')return c-'A'+10;
    return -1;
}

static int hex_decode(const char *hex, unsigned char **out, size_t *out_len){
    size_t n = strlen(hex);
    if(n % 2 != 0) return 0;

    *out = (unsigned char*)malloc(n/2);
    if(!*out) return 0;

    for(size_t i=0; i<n; i+=2){
        int hi = from_hex(hex[i]), lo = from_hex(hex[i+1]);
        if(hi < 0 || lo < 0){
            free(*out);
            *out = NULL;
            return 0;
        }
        (*out)[i/2] = (unsigned char)((hi<<4) | lo);
    }
    *out_len = n/2;
    return 1;
}

// Split input hex string into malloc'd NONCE_HEX, CT_HEX, TAG_HEX.
// Layout: [nonce(24)][ciphertext(variable)][tag(32)]
static int split_nonce_ct_tag(const char *input_hex,
                             char **nonce_hex, char **ct_hex, char **tag_hex)
{
    if(!input_hex || !nonce_hex || !ct_hex || !tag_hex) return 0;

    size_t n = strlen(input_hex);

    // Must be at least nonce + tag
    if(n < (size_t)(NONCE_HEX_LEN + TAG_HEX_LEN)) return 0;

    // Whole thing must be even-length hex
    if(n % 2 != 0) return 0;

    size_t ct_len = n - (size_t)(NONCE_HEX_LEN + TAG_HEX_LEN);
    // ciphertext hex must also be even length
    if(ct_len % 2 != 0) return 0;

    *nonce_hex = (char*)malloc(NONCE_HEX_LEN + 1);
    *tag_hex   = (char*)malloc(TAG_HEX_LEN + 1);
    *ct_hex    = (char*)malloc(ct_len + 1);

    if(!*nonce_hex || !*tag_hex || !*ct_hex){
        free(*nonce_hex); free(*tag_hex); free(*ct_hex);
        *nonce_hex = *tag_hex = *ct_hex = NULL;
        return 0;
    }

    memcpy(*nonce_hex, input_hex, NONCE_HEX_LEN);
    (*nonce_hex)[NONCE_HEX_LEN] = '\0';

    memcpy(*ct_hex, input_hex + NONCE_HEX_LEN, ct_len);
    (*ct_hex)[ct_len] = '\0';

    memcpy(*tag_hex, input_hex + (n - TAG_HEX_LEN), TAG_HEX_LEN);
    (*tag_hex)[TAG_HEX_LEN] = '\0';

    return 1;
}

int main(int argc, char **argv){
    // Usage: ./aes_gcm_decrypt_split <nonce||ciphertext||tag as hex>
    if(argc != 2){
        fprintf(stderr,
            "Usage: %s <HEXSTRING>\n"
            "Where HEXSTRING = NONCE(24 hex) + CIPHERTEXT(variable hex) + TAG(32 hex)\n",
            argv[0]
        );
        return 1;
    }

    const char *COMBINED_HEX = argv[1];

    // =========================================================
    // HARD-CODE KEY + OPTIONAL AAD HERE
    // =========================================================
    const char *KEY_HEX = "a1b2c3d4e5f6789012345678901234567890abcdef1234567890abcdef123456";
    const char *AAD_HEX = ""; // leave "" if none

    // Split combined into the three pieces
    char *NONCE_HEX = NULL;
    char *CT_HEX    = NULL;
    char *TAG_HEX   = NULL;

    if(!split_nonce_ct_tag(COMBINED_HEX, &NONCE_HEX, &CT_HEX, &TAG_HEX)){
        fprintf(stderr,
            "Error: input must be even-length hex and at least %d chars total.\n"
            "Expected: [nonce 24 hex][ciphertext ...][tag 32 hex]\n",
            NONCE_HEX_LEN + TAG_HEX_LEN
        );
        return 1;
    }

    // =========================================================
    // decode inputs
    // =========================================================
    unsigned char *key=NULL,*nonce=NULL,*tag=NULL,*aad=NULL,*ct=NULL;
    size_t key_len=0, nonce_len=0, tag_len=0, aad_len=0, ct_len=0;

    if(!hex_decode(KEY_HEX, &key, &key_len) || key_len!=32){
        fprintf(stderr,"Key must be 32 bytes (64 hex chars)\n");
        return 1;
    }
    if(!hex_decode(NONCE_HEX, &nonce, &nonce_len) || nonce_len!=12){
        fprintf(stderr,"Nonce must be 12 bytes (24 hex chars)\n");
        return 1;
    }
    if(!hex_decode(TAG_HEX, &tag, &tag_len) || tag_len!=16){
        fprintf(stderr,"Tag must be 16 bytes (32 hex chars)\n");
        return 1;
    }
    if(AAD_HEX && AAD_HEX[0] != '\0'){
        if(!hex_decode(AAD_HEX, &aad, &aad_len)){
            fprintf(stderr,"Invalid AAD hex\n");
            return 1;
        }
    }
    if(!hex_decode(CT_HEX, &ct, &ct_len)){
        fprintf(stderr,"Invalid ciphertext hex\n");
        return 1;
    }

    // =========================================================
    // decrypt
    // =========================================================
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if(!ctx){
        fprintf(stderr,"CTX new failed\n");
        return 1;
    }

    int ok = 1;
    ok &= EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL);
    ok &= EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, (int)nonce_len, NULL);
    ok &= EVP_DecryptInit_ex(ctx, NULL, NULL, key, nonce);
    if(!ok){
        fprintf(stderr,"DecryptInit failed\n");
        return 1;
    }

    unsigned char *pt = (unsigned char*)malloc(ct_len ? ct_len : 1);
    if(!pt){
        fprintf(stderr,"OOM\n");
        return 1;
    }

    int outl=0, tot=0;

    // Supply AAD (must match exactly what was used in encryption)
    if(aad && aad_len>0){
        if(!EVP_DecryptUpdate(ctx, NULL, &outl, aad, (int)aad_len)){
            fprintf(stderr,"AAD update failed\n");
            return 1;
        }
    }

    // Decrypt ciphertext
    if(!EVP_DecryptUpdate(ctx, pt, &outl, ct, (int)ct_len)){
        fprintf(stderr,"DecryptUpdate failed\n");
        return 1;
    }
    tot = outl;

    // Set expected tag BEFORE final
    if(!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, (int)tag_len, tag)){
        fprintf(stderr,"SET_TAG failed\n");
        return 1;
    }

    // Finalize (verifies tag)
    if(!EVP_DecryptFinal_ex(ctx, pt + tot, &outl)){
        fprintf(stderr,"DecryptFinal: authentication failed (bad key/nonce/tag/AAD or corrupted data)\n");
        return 2;
    }
    tot += outl;

    // Output plaintext (raw bytes + newline)
    fwrite(pt, 1, (size_t)tot, stdout);
    fputc('\n', stdout);

    // cleanup
    free(pt); free(ct); free(aad); free(tag); free(nonce); free(key);
    free(NONCE_HEX); free(CT_HEX); free(TAG_HEX);
    EVP_CIPHER_CTX_free(ctx);
    return 0;
}