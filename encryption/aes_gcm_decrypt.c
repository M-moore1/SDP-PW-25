// aes_gcm_decrypt.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>



// Mason


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
    *out_len=n/2; return 1;
}
static char *read_all_stdin_text(void){
    size_t cap=4096, n=0;
    char *buf=(char*)malloc(cap);
    if(!buf) return NULL;
    size_t r;
    while((r=fread(buf+n,1,cap-n,stdin))>0){
        n+=r;
        if(n==cap){ cap*=2; char *tmp=(char*)realloc(buf,cap); if(!tmp){ free(buf); return NULL; } buf=tmp; }
    }
    buf[n]='\0';
    // Strip trailing newlines/spaces
    while(n>0 && (buf[n-1]=='\n'||buf[n-1]=='\r'||buf[n-1]==' '||buf[n-1]=='\t')) buf[--n]='\0';
    return buf;
}

int main(int argc, char **argv){
    if(argc<4 || argc>5){
        fprintf(stderr,"Usage: %s KEY_HEX(32B) NONCE_HEX(12B) TAG_HEX(16B) [AAD_HEX]\n", argv[0]);
        return 1;
    }
    unsigned char *key=NULL,*nonce=NULL,*tag=NULL,*aad=NULL,*ct=NULL;
    size_t key_len=0, nonce_len=0, tag_len=0, aad_len=0, ct_len=0;

    if(!hex_decode(argv[1], &key, &key_len) || key_len!=32){ fprintf(stderr,"Key must be 32 bytes\n"); return 1; }
    if(!hex_decode(argv[2], &nonce, &nonce_len) || nonce_len!=12){ fprintf(stderr,"Nonce must be 12 bytes\n"); return 1; }
    if(!hex_decode(argv[3], &tag, &tag_len) || tag_len!=16){ fprintf(stderr,"Tag must be 16 bytes\n"); return 1; }
    if(argc==5 && !hex_decode(argv[4], &aad, &aad_len)){ fprintf(stderr,"Invalid AAD hex\n"); return 1; }

    char *ct_hex_text = read_all_stdin_text();
    if(!ct_hex_text){ fprintf(stderr,"Failed to read ciphertext hex from stdin\n"); return 1; }
    if(!hex_decode(ct_hex_text, &ct, &ct_len)){ fprintf(stderr,"Invalid ciphertext hex\n"); free(ct_hex_text); return 1; }
    free(ct_hex_text);

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if(!ctx){ fprintf(stderr,"CTX new failed\n"); return 1; }

    int ok = EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL);
    ok &= EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, (int)nonce_len, NULL);
    ok &= EVP_DecryptInit_ex(ctx, NULL, NULL, key, nonce);
    if(!ok){ fprintf(stderr,"DecryptInit failed\n"); return 1; }

    int outl=0, tot=0;
    unsigned char *pt = (unsigned char*)malloc(ct_len);
    if(!pt){ fprintf(stderr,"OOM\n"); return 1; }

    if(aad && aad_len>0){
        if(!EVP_DecryptUpdate(ctx, NULL, &outl, aad, (int)aad_len)){
            fprintf(stderr,"AAD update failed\n"); return 1;
        }
    }
    if(!EVP_DecryptUpdate(ctx, pt, &outl, ct, (int)ct_len)){
        fprintf(stderr,"DecryptUpdate failed\n"); return 1;
    }
    tot = outl;

    if(!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, (int)tag_len, tag)){
        fprintf(stderr,"SET_TAG failed\n"); return 1;
    }
    if(!EVP_DecryptFinal_ex(ctx, pt + tot, &outl)){
        fprintf(stderr,"DecryptFinal: authentication failed (bad key/nonce/tag/AAD or corrupted data)\n");
        return 2;
    }
    tot += outl;

    fwrite(pt, 1, (size_t)tot, stdout);
    fputc('\n', stdout);

    free(pt); free(ct); EVP_CIPHER_CTX_free(ctx);
    free(aad); free(tag); free(nonce); free(key);
    return 0;
}
