#include<stdio.h>
#include<sys/socket.h>
#include<strings.h>
#include<string.h>
#include<errno.h>
#include<sys/un.h>
#include<sys/stat.h>
#include<unistd.h>
#include<fcntl.h>
#include"crypt.h"
#define PAGE_SIZE (1<<12)

int main(int argc,char **argv)
{
	unsigned char data[PAGE_SIZE];
	unsigned char encryptData[PAGE_SIZE];
	unsigned char publicKey[KEY_SIZE];
	struct sockaddr_un server;
	int sockfd,tmp,fd,length;	

	if (argc!=2){
		printf("useage:./a.out sockaddr\n");
		goto out;
	}
	
	if ((sockfd=socket(PF_UNIX,SOCK_STREAM,0))<0){
		printf("socket error!\n");
		goto out;
	}
	bzero(&server,sizeof(server));
	server.sun_family=PF_UNIX;
	snprintf(server.sun_path,sizeof(server.sun_path),"%s",argv[1]);
	if (connect(sockfd,(struct sockaddr *)&server,sizeof(server))<0){
		printf("%s\n",strerror(errno));
		goto clear_sockfd;
	}
	
	if ((fd=open("./bin",O_RDONLY))<0){
		printf("open app error!\n");
		goto clear_sockfd;
	}
	tmp=read(fd,data,512);
//	printf("read len:%d\n",tmp);
	/*
	* now get public key
	*/
	read(sockfd,publicKey,KEY_SIZE);
	/*
	* now encrypt
	*/
	if ((length=public_encrypt(tmp,data,encryptData,publicKey))<0){
		printf("encrypt failed!\n");
		goto clear_sockfd;
	}
//	printf("encrypt len:%d\n",length);
	write(sockfd,encryptData,length);
	printf("write success!\n");
	close(sockfd);
	return 0;

clear_sockfd:
	close(sockfd);
out:
	return -1;
}
