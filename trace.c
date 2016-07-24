#include<stdio.h>
#include<sys/ptrace.h>
#include<unistd.h>
#include<sys/wait.h>
#include<errno.h>
#include<string.h>
#include<stdlib.h>
#include<sys/syscall.h>
#include<sys/user.h>
#include<strings.h>
void getdata(pid_t pid,long addr, char *str,int len){
	int i;
	long data;
	for(i=0;i<len/sizeof(long);i++){
		data=ptrace(PTRACE_PEEKDATA,pid,addr+i*sizeof(long),NULL);
		*(long *)(str+i*sizeof(long))=data;
	}
	*(unsigned char*)(str+10)='Z';

	for(i=0;i<len/sizeof(long);i++){
		data=*(long *)(str+i*sizeof(long));
		ptrace(PTRACE_POKEDATA,pid,addr+i*sizeof(long),data);
	}
}
int main(int argc,char **argv){
	pid_t pid;
	int status,buf_len,i;
	struct user_regs_struct regs;
	char *buf;
	int insyscall=0;
	static long once=0;
	
	if (argc<2){
		printf("Usage:./a.out prog <args>\n");
		return 1;
	}
	if ((pid=fork())==0){
		ptrace(PTRACE_TRACEME,0,NULL,NULL);
		status=execvp(argv[1],argv+1);
		if(status<0){
			printf("execvp %s error:%s\n",argv[1],strerror(errno));
			return -1;
		}
	}else if(pid>0){		
		while(1){
			wait(&status);
			if(WIFEXITED(status))
				break;
			ptrace(PTRACE_GETREGS, pid, 0, &regs);
			if (regs.orig_eax!=SYS_write){
				ptrace(PTRACE_SYSCALL,pid,NULL,NULL);
				continue;
			}
			/*		
			if (regs.ebx!=0){
				ptrace(PTRACE_SYSCALL,pid,NULL,NULL);
				continue;
			}
			*/
			if (insyscall==0){
				if (!once){
					once=regs.ebx;
				}
				if(once!=regs.ebx){
					ptrace(PTRACE_SYSCALL,pid,NULL,NULL);
					continue;
				}
				buf_len=(regs.edx+sizeof(long)-1)/sizeof(long)*sizeof(long);
				if((buf=(char *)malloc(buf_len))==NULL){
					printf("malloc error!\n");
					return 1;
				}
				bzero(buf,buf_len);
				getdata(pid,regs.ecx,buf,buf_len);
				free(buf);
				ptrace(PTRACE_SYSCALL,pid,NULL,NULL);
				insyscall=1;
			}else{
				ptrace(PTRACE_SYSCALL,pid,NULL,NULL);
				insyscall=0;
			}
		}
		return 0;
	}else{
		printf("fork error!\n");
		return 1;
	}
}
