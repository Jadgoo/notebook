#include<openssl/pem.h>
#include<openssl/ssl.h>
#include<openssl/rsa.h>
#include<openssl/evp.h>
#include<openssl/bio.h>
#include<openssl/err.h>
#include<stdio.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#define BUF_SIZE 1024
 
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
 
int public_encrypt(unsigned char * data,int data_len,unsigned char * key, unsigned char *encrypted)
{
    RSA * rsa = createRSA(key,1);
    int result = RSA_public_encrypt(data_len,data,encrypted,rsa,RSA_PKCS1_PADDING);
    return result;
}
int private_decrypt(unsigned char * enc_data,int data_len,unsigned char * key, unsigned char *decrypted)
{
    RSA * rsa = createRSA(key,0);
    int  result = RSA_private_decrypt(data_len,enc_data,decrypted,rsa,RSA_PKCS1_PADDING);
    return result;
}
 
 
int private_encrypt(unsigned char * data,int data_len,unsigned char * key, unsigned char *encrypted)
{
    RSA * rsa = createRSA(key,0);
    int result = RSA_private_encrypt(data_len,data,encrypted,rsa,RSA_PKCS1_PADDING);
    return result;
}
int public_decrypt(unsigned char * enc_data,int data_len,unsigned char * key, unsigned char *decrypted)
{
    RSA * rsa = createRSA(key,1);
    int  result = RSA_public_decrypt(data_len,enc_data,decrypted,rsa,RSA_PKCS1_PADDING);
    return result;
}
 
void printLastError(char *msg)
{
    char * err = malloc(130);;
    ERR_load_crypto_strings();
    ERR_error_string(ERR_get_error(), err);
    printf("%s ERROR: %s\n",msg, err);
    free(err);
}
 
int main()
{
	char publicKey[BUF_SIZE];
	char privateKey[BUF_SIZE];
	int fd;
	if ((fd=open("./rsa_private_key.pem",O_RDONLY))<0){
		printf("open private key error!\n");
		return -1;
	}
	read(fd,privateKey,BUF_SIZE);
	if ((fd=open("./rsa_public_key.pem",O_RDONLY))<0){
		printf("open public key error!\n");
		return -1;
	}
	read(fd,publicKey,BUF_SIZE);
 
	char plainText[] = "Hello this is Raviaaaaaaaaa";
    
	unsigned char  encrypted[4098]={};
	unsigned char decrypted[4098]={};
	 
	int encrypted_length= public_encrypt(plainText,strlen(plainText),publicKey,encrypted);
	if(encrypted_length == -1)
	{
		printLastError("Public Encrypt failed ");
		exit(0);
	}
	printf("Encrypted length =%d\n",encrypted_length);
	 
	int decrypted_length = private_decrypt(encrypted,encrypted_length,privateKey, decrypted);
	if(decrypted_length == -1)
	{
		printLastError("Private Decrypt failed ");
		exit(0);
	}
	printf("Decrypted Text =%s\n",decrypted);
	printf("Decrypted Length =%d\n",decrypted_length);
	 
	 
	encrypted_length= private_encrypt(plainText,strlen(plainText),privateKey,encrypted);
	if(encrypted_length == -1)
	{
		printLastError("Private Encrypt failed");
		exit(0);
	}
	printf("Encrypted length =%d\n",encrypted_length);
	 
	decrypted_length = public_decrypt(encrypted,encrypted_length,publicKey, decrypted);
	if(decrypted_length == -1)
	{
		printLastError("Public Decrypt failed");
		exit(0);
	}
	printf("Decrypted Text =%s\n",decrypted);
	printf("Decrypted Length =%d\n",decrypted_length); 
}
