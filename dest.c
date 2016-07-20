#include<stdio.h>
#include<unistd.h>
#include"unistd_32.h"
#include"userfaultfd.h"
#include<pthread.h>
#include<sys/ioctl.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<strings.h>
#include<string.h>
#include<errno.h>
#include<sys/un.h>
#include<sys/mman.h>
#define PAGE_SIZE (1<<12)
#define FAULT_ADDR 0x90000000

char *dst;
int dst_len=PAGE_SIZE;

int faultfd_api(int faultfd)
{
	struct uffdio_api uffdio_api;

	uffdio_api.api=UFFD_API;
	uffdio_api.features=0;
	if (ioctl(faultfd,UFFDIO_API,&uffdio_api)){
		printf("API errpr!\n");
		return -1;
	}
	return 0;
}

int faultfd_reg_region(int faultfd, void *dst, size_t len)
{
	struct uffdio_register uffdio_register;

	uffdio_register.range.start=(unsigned long)dst;
	uffdio_register.range.len=len;
	uffdio_register.mode=UFFDIO_REGISTER_MODE_MISSING;
	if (ioctl(faultfd,UFFDIO_REGISTER,&uffdio_register)){
		printf("register error!\n");
		return -1;
	}
	return 0;
}

int faultfd_copy(int faultfd,void *dst,void *src,size_t len)
{
	struct uffdio_copy uffdio_copy;
	int ret;

	uffdio_copy.dst=(unsigned long)dst;
	uffdio_copy.src=(unsigned long)src;
	uffdio_copy.len=len;
	uffdio_copy.mode=0;
	if (ioctl(faultfd,UFFDIO_COPY,&uffdio_copy)){
		printf("copy error!\n");
		return -1;
	}
	return 0;
}

void *fill_memory(void *arg)
{
	char src[PAGE_SIZE];
	int faultfd=*(int *)arg;
	char path[]="/tmp/faultfd-XXXXXX";
	int sockfd,acceptfd,tmp;
	struct sockaddr_un server;
		
	if (!mkdtemp(path)){
		printf("make path %s error:%s\n",path,strerror(errno));
		goto out;
	}
	
	printf("%s/sock\n",path);
	if ((sockfd=socket(PF_UNIX,SOCK_STREAM,0))<0){
		printf("socket error!\n");
		goto clear_tmpfs;
	}
	bzero(&server,sizeof(server));
	server.sun_family=PF_UNIX;
	snprintf(server.sun_path,sizeof(server.sun_path),"%s/sock",path);

	do{
		tmp=bind(sockfd,(struct sockaddr *)&server,sizeof(server));
	}while(tmp<0 && errno==EINTR);
	if (listen(sockfd,1)<0){
		printf("listen error!\n");
		goto clear_sockfd;
	}
	tmp=sizeof(server);
	do{
		acceptfd=accept(sockfd,(struct sockaddr *)&server,&tmp);
	}while(acceptfd<0 && errno==EINTR);
	if (acceptfd<0){
		printf("accept error!\n");
		goto clear_sockfd;
	}
	read(acceptfd,src,PAGE_SIZE);
	if (faultfd_copy(faultfd,dst,src,dst_len)){
		goto clear_acceptfd;
	}
	close(acceptfd);
	unlink(server.sun_path);
	close(sockfd);
	rmdir(path);
	return (void *)1;

clear_acceptfd:
	close(acceptfd);
clear_sockfd:
	unlink(server.sun_path);
	close(sockfd);
clear_tmpfs:
	rmdir(path);
out:
	return NULL;
}

int main(int argc,char **argv)
{
	pthread_t fill_thread;
	int faultfd;
	void *ret;
	int tmp;

/*
	if (posix_memalign((void **)&dst,PAGE_SIZE,dst_len)){
		printf("init memory error!\n");
		return -1;
	}
*/
	if (!(dst=mmap((void *)FAULT_ADDR,PAGE_SIZE,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS,-1,0))){
		printf("mmap error!\n");
		return -1;
	}
/*
	if (mprotect(dst,dst_len,PROT_READ|PROT_WRITE|PROT_EXEC)){
		printf("mprotect error!\n");
		return -1;
	}
*/
	printf("memory at:%p\n",dst);

	if ((faultfd=syscall(__NR_userfaultfd, 0))<0){
		printf("userfaultfd syscall error!\n");
		return -1;
	}
	if (faultfd_api(faultfd)){
		return -1;
	}
	if (faultfd_reg_region(faultfd,dst,dst_len)){
		return -1;
	}
	if (pthread_create(&fill_thread,NULL,fill_memory,(void *)&faultfd)){
		printf("create thread error!\n");
		return -1;
	}
	/*
	 * be careful of 'printf' buffer, '\n' will flush.
	 */
	tmp=(*(int (*)())dst)();
//	printf("%c\n",*(char *)dst);
	printf("result is %d\n",tmp);
//	printf("%d\n",getpid());
	if (pthread_join(fill_thread,&ret)){
		printf("wait thread error!\n");
		return -1;
	}
	if(!(int)ret){
		printf("inner thread error!\n");
		return -1;
	}
//	sleep(50);
	close(faultfd);
//	free(dst);
	return 0;
}
