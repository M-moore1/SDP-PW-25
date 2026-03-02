// aes_gcm_decrypt_hardcoded.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>

// ---------- helpers ----------
static int from_hex(char c){
    if(c>='0'&&c<='9')return c-'0';
    if(c>='a'&&c<='f')return c-'a'+10;
    if(c>='A'&&c<='F')return c-'A'+10;
    return -1;
}
static int hex_decode(const char *hex, unsigned char **out, size_t *out_len){
    size_t n=strlen(hex);
    if(n%2!=0) return 0;
    *out=(unsigned char*)malloc(n/2);
    if(!*out) return 0;
    for(size_t i=0;i<n;i+=2){
        int hi=from_hex(hex[i]), lo=from_hex(hex[i+1]);
        if(hi<0||lo<0){ free(*out); return 0; }
        (*out)[i/2]=(unsigned char)((hi<<4)|lo);
    }
    *out_len=n/2;
    return 1;
}

// Optional: print bytes as hex (useful for debugging)
static void print_hex(const char *label, const unsigned char *buf, size_t len){
    fprintf(stderr, "%s (%zu bytes): ", label, len);
    for(size_t i=0;i<len;i++) fprintf(stderr, "%02x", buf[i]);
    fprintf(stderr, "\n");
}

int main(void){
    // =========================================================
    // HARD-CODE TEST VECTORS HERE (hex strings, no 0x prefix)
    // =========================================================

    // AES-256 key (32 bytes = 64 hex chars)
    const char *KEY_HEX   = "a1b2c3d4e5f6789012345678901234567890abcdef1234567890abcdef123456";

    // GCM nonce/IV (12 bytes = 24 hex chars) is most common
    const char *NONCE_HEX = "bd6665607407c261687a7e62";

    // Auth tag (16 bytes = 32 hex chars) is most common
    const char *TAG_HEX   = "bb2153d5359d004b722e1ab4c51dd0e6";

    // Ciphertext bytes (any length)
    // NOTE: this should be ONLY the ciphertext (not including tag)
    const char *CT_HEX    = "64036aa70f8529132a17b4c393ea269f6c03fb5b0d3026e22e7fbd428fbdebe8c688d420342cb020e68560da9f80771c28154ee9eaf171f82fc06f002cc60f4a1aee3cb4271fe1cade7f8005ecccadfc0ddbe2eee078";  // <-- replace with your real ciphertext hex

    // Optional AAD (can be empty string if none)
    // If you didn't use AAD when encrypting, leave AAD_HEX = "" and it will be skipped.
    const char *AAD_HEX   = "";

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

    // (Optional) show what we parsed
    // print_hex("key", key, key_len);
    // print_hex("nonce", nonce, nonce_len);
    // print_hex("tag", tag, tag_len);
    // print_hex("ct", ct, ct_len);
    // if(aad) print_hex("aad", aad, aad_len);

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
    EVP_CIPHER_CTX_free(ctx);
    return 0;
}