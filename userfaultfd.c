#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "unistd_32.h"
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include "userfaultfd.h"

#define PAGE_SIZE	(1 << 12)

static void faultfd_api(int faultfd)
{
	struct uffdio_api uffdio_api;

	uffdio_api.api = UFFD_API;
	uffdio_api.features = 0;

	if (ioctl(faultfd, UFFDIO_API, &uffdio_api)) {
		perror("API ioctl failed");
		exit(-1);
	}
}

static void faultfd_reg_region(int faultfd, void *dst, size_t len)
{
	struct uffdio_register uffdio_register;

	uffdio_register.range.start = (unsigned long)dst;
	uffdio_register.range.len = len;
	uffdio_register.mode = UFFDIO_REGISTER_MODE_MISSING;
	if (ioctl(faultfd, UFFDIO_REGISTER, &uffdio_register)) {
		perror("register ioctl failed");
		exit(-1);
	}
}

static int faultfd_copy(int faultfd, void *dst, void *src, size_t len)
{
	struct uffdio_copy uffdio_copy;
	int ret;

again:
	uffdio_copy.dst = (__u64)dst;
	uffdio_copy.src = (__u64)src;
	uffdio_copy.len = (__u64)len;
	/* wakeup page fault. */
	uffdio_copy.mode = 0;

	ret = ioctl(faultfd, UFFDIO_COPY, &uffdio_copy);
/*
	if (errno == EAGAIN) {
		int copy = uffdio_copy.copy;

		dst += copy;
		src += copy;
		len -= copy;
		if (len <= 0) {
			printf("len is incorrect.\n");
			return -1;
		}

		goto again;
	}
*/
	if (ret < 0) {
		perror("copy ioctl failed");
		return -1;
	}

	return 0;
}

static char *dst;
static int dst_len = 10 * PAGE_SIZE;
static int ready;

static void *fill_memory(void *arg)
{
	char src[PAGE_SIZE];
	struct uffd_msg msg;
	int i;
	int faultfd = *(int *)arg;


	for (i = 0; i < PAGE_SIZE; i++)
		src[i] = 0x99;

	ready = 1;

	while (1) {
		if (read(faultfd, &msg, sizeof(msg)) < 0) {
			printf("read fault data failed");
			return NULL;
		}

		if (msg.event != UFFD_EVENT_PAGEFAULT) {
			printf("unexpected event %d.\n", msg.event);
			return NULL;
		}

		/* we just read the region, no WRITE, no WP. */
		if (msg.arg.pagefault.flags != 0) {
			printf("unexpected flags %llx.\n", msg.arg.pagefault.flags);
			return NULL;
		}
		if (faultfd_copy(faultfd, (void *)(msg.arg.pagefault.address & ~(PAGE_SIZE - 1)),
				 src, PAGE_SIZE))
			return NULL;		
	}

	return NULL;
}

static void read_memory(void)
{
	int i;

	while (!ready) ;;

	for (i = 0; i < dst_len; i++) {
		if (dst[i] != (char)0x99) {
			printf("data check failed, value = %x, expect %x.\n",
			       dst[i], 0x99);
			return;
		}
	}
	printf("memory validation passed.\n");
}


int main(int argc, char *argv[])
{
	pthread_t fill_thread;
	int faultfd;

	printf("running...\n");
  
	/* init dst memory region. */
	if (posix_memalign((void **)&dst, PAGE_SIZE, dst_len)) {
		perror("init dst memory failed");
		return -1;
	}

	/* tell kernel to reclaim its memory. */
/*
	if (madvise(dst, dst_len, MADV_DONTNEED)) {
		perror("madvise failed");
		return -1;
	}
*/
	/* 1st step: do userfalutfd system call. */
	faultfd = syscall(__NR_userfaultfd, 0);
	if (faultfd < 0) {
		perror("userfaultfd syscall failed");
		return -1;
	}

	/* 2nd step: API check. */
	faultfd_api(faultfd);
        
        /* 3rd step: register region. */
	faultfd_reg_region(faultfd, dst, dst_len);

        /* a separate thread fills memory in the background */
	if (pthread_create(&fill_thread, NULL, fill_memory, (void *)&faultfd)) {
		perror("create fill thread failed");
		return -1;
	}

        /* memory access on the region. */
	read_memory();

        pthread_cancel(fill_thread);
	close(faultfd);

	if (pthread_join(fill_thread, NULL)) {
		perror("wait fill thread failed");
		return -1;
	}

	return 0;
}
