#include<openssl/pem.h>
#include<openssl/ssl.h>
#include<openssl/rsa.h>
#include<openssl/evp.h>
#include<openssl/bio.h>
#include<openssl/err.h>
#include<stdio.h>

/*
BIO *keypub,*keypriv;

int generate_key(void)
{
	int ret=-1;
	RSA *rsa= NULL;
	keypub=NULL;
	keypriv=NULL;

    keypub = BIO_new(BIO_s_mem());
    keypriv = BIO_new(BIO_s_mem());
    if (!keypub || !keypriv){
    	printf("create bio file error!\n");
    	goto error;
    }
    rsa = RSA_generate_key( 1024, RSA_F4, NULL, NULL);
    if (!rsa){
    	if (!PEM_write_bio_RSAPublicKey(keypub, rsa)){
    		printf("create public key error!\n");
    		goto error;
    	}
    	if (!PEM_write_bio_RSAPrivateKey(keypriv, rsa, NULL, NULL, 0, NULL, NULL)){
     		printf("create private key error!\n");
    		goto error;
    	}
    }
    ret = 0;
    goto out;
    
error:
	if (keypub)
		BIO_free(keypub);
	if (keypriv)
		BIO_free(keypriv);
	if (rsa) 
		RSA_free(rsa);
out:	
    return ret;
}
*/
RSA * createRSA(unsigned char * key,int public)
{
    RSA *rsa= NULL;
    BIO *keybio;

    keybio = BIO_new_mem_buf(key, -1);
    if (!keybio){
        printf("create key BIO error!\n");
        return NULL;
    }
    if(public){
        rsa = PEM_read_bio_RSA_PUBKEY(keybio,&rsa,NULL,NULL);
    }else{
        rsa = PEM_read_bio_RSAPrivateKey(keybio,&rsa,NULL,NULL);
    }

    if(!rsa){
        printf("create RSA error!\n");
    }
    return rsa;
}

int public_encrypt(int src_len, void *src,void *dest, void *key)
{
    RSA * rsa = createRSA(key,1);
    int result = RSA_public_encrypt(src_len,src,dest,rsa,RSA_PKCS1_PADDING);
    return result;
}
int private_decrypt(int src_len, void *src,void *dest, void *key)
{
    RSA * rsa = createRSA(key,0);
    int  result = RSA_private_decrypt(src_len,src,dest,rsa,RSA_PKCS1_PADDING);
    return result;
}
int private_encrypt(int src_len, void *src,void *dest, void *key)
{
    RSA * rsa = createRSA(key,0);
    int result = RSA_private_encrypt(src_len,src,dest,rsa,RSA_PKCS1_PADDING);
    return result;
}
int public_decrypt(int src_len, void *src,void *dest, void *key)
{
    RSA * rsa = createRSA(key,1);
    int  result = RSA_public_decrypt(src_len,src,dest,rsa,RSA_PKCS1_PADDING);
    return result;
}
