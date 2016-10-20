#include <stdio.h>
#include <stdlib.h>
#include <openssl/obj_mac.h>
#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <openssl/conf.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#define secp128r1 16
#define secp192r1 24
#define secp256r1 32
#define secp384r1 48
#define ECC_CURVE secp256r1

#if (ECC_CURVE != secp128r1 && ECC_CURVE != secp192r1 && ECC_CURVE != secp256r1 && ECC_CURVE != secp384r1)
    #error "Must define ECC_CURVE to one of the available curves"
#endif

#define NUM_ECC_DIGITS ECC_CURVE

#ifdef __cplusplus
extern "C"{
#endif

int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key, unsigned char *iv, unsigned char *ciphertext);
int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key, unsigned char *iv, unsigned char *plaintext);
size_t calcDecodeLength(const char* b64input);
int Base64Decode(char* b64message, unsigned char** buffer, size_t* length) ;
int Base64Encode(const unsigned char* buffer, size_t length, char** b64text) ;
void deriveSecret (uint8_t stpubx[],uint8_t stpuby[], uint8_t lcpriv[],uint8_t lcpubx[],  uint8_t lcpuby[], uint8_t secret[]);

#ifdef __cplusplus
} // extern "C"
#endif
