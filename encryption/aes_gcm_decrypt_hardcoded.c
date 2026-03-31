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


    // bd6665607407c261687a7e6264036aa70f8529132a17b4c393ea269f6c03fb5b0d3026e22e7fbd428fbdebe8c688d420342cb020e68560da9f80771c28154ee9eaf171f82fc06f002cc60f4a1aee3cb4271fe1cade7f8005ecccadfc0ddbe2eee078bb2153d5359d004b722e1ab4c51dd0e6

    // NEW W COMMAND
    //1ef4ceb4196504bf748d026f7e5be378dc5a9b39edfdd4341d3e9d2eef124eb5aaf40069b5dd0b467959fbe3a3d87ddb0a7bdbdb6d4b29d4ed9de5ecc07067dd95fe10f48b54433e96647f3b34142ec1c95bd972b324430217d51824fa56eee884f690627f5cfb2c08810b4f731da4a46b0d28707f4a51c022a6399dab86851e8dba33e272ab6162a36a73e979c573e537559e824a78f1d1636ad972

    // AES-256 key (32 bytes = 64 hex chars)
    const char *KEY_HEX   = "a1b2c3d4e5f6789012345678901234567890abcdef1234567890abcdef123456";

    // GCM nonce/IV (12 bytes = 24 hex chars) is most common
    const char *NONCE_HEX = "373ab9ca3fb48c50c0c4fa23";

    // Auth tag (16 bytes = 32 hex chars) is most common
    const char *TAG_HEX   = "c26ff8380ebb8aeb605888f18737bcfa";

    // Ciphertext bytes (any length)
    // NOTE: this should be ONLY the ciphertext (not including tag)
    const char *CT_HEX    = "3a9dea35cd482ad27526cf8349b1ab5ebb889d2e637e24e73ab2b4476f826e76b6deb716d530fd2fdcb53ad40569298a6f8fc6cd3f21e9a2182fb09a120e5c5a5e923b694caecbe297483c594c0f9a25cd2195cad34afe4f2b1b7241487bfe403f5ced6d02d94bedf1f0c004220f3b01b3085ec8b691edf2d4526ba601e9d2b4";  // <-- replace with your real ciphertext hex

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