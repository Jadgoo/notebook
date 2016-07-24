#include<openssl/pem.h>
#include<openssl/ssl.h>
#include<openssl/rsa.h>
#include<openssl/evp.h>
#include<openssl/bio.h>
#include<openssl/err.h>
extern int generate_key(void);
extern int public_encrypt(int src_len, void *src,void *dest, void *key);
extern int private_decrypt(int src_len, void *src,void *dest, void *key);
extern int private_encrypt(int src_len, void *src,void *dest, void *key);
extern int public_decrypt(int src_len, void *src,void *dest, void *key);
