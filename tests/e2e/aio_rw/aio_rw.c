#include <linux/aio_abi.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

const char* info = "Welcome to ASYNC world!";

void main() {
	aio_context_t ctx = 0;
	
	int err = syscall(__NR_io_setup, 10, &ctx);
	if (err < 0) {
		printf("io_setup err: %d\n", errno);
		return;
	}
	
	printf("io_setup success\n");
	
	err = open("test.txt", O_CREAT | O_RDWR, 0777);
	if (err < 0) {
		printf("open err: %d\n", errno);
		return;
	}
	
	printf("open success\n");
	
	int fd = err;
	struct iocb req;
	memset(&req, 0, sizeof(req));
	
	req.aio_data = 0xDEADBEEF;
	req.aio_lio_opcode = IOCB_CMD_PWRITE;
	req.aio_fildes = fd;
	req.aio_buf = (__u64)info;
	req.aio_nbytes = strlen(info);
	req.aio_offset = 0;
	
	struct iocb* reqs[1];
	reqs[0] = &req;
	
	err = syscall(__NR_io_submit, ctx, 1, &reqs);
	if (err < 0) {
		printf("io_submit err: %d\n", errno);
		return;
	} else if (err == 0) {
		printf("io_submit reports 0 successful IOCBs, but no error code. errno is %d", errno);
	}
	
	printf("io_submit success: %d\n", err);
	
	struct io_event evt;
	
	do {
		err = syscall(__NR_io_getevents, ctx, 0, 1, &evt, NULL);
		if (err < 0) {
			printf("io_getevents err: %d\n", errno);
			return;
		}
	} while (err < 1);
	
	printf("io_getevents success: %d\n", err);
	printf("evt.data: 0x%llX\n", evt.data);
	
	if (evt.obj == (__u64)&req) {
		printf("evt.obj matches &req\n");
	} else {
		printf("evt.obj does NOT match &req, 0x%llX given\n", evt.obj);
		printf("(&req is 0x%llX)\n", (__u64)&req);
	}
	
	printf("evt.res:  %lld\n", evt.res);
	printf("evt.res2: %lld\n", evt.res2);
	
	memset(&req, 0, sizeof(req));
	char rbuf[513];
	
	req.aio_data = 0xCAFEBABE;
	req.aio_lio_opcode = IOCB_CMD_PREAD;
	req.aio_fildes = fd;
	req.aio_buf = (__u64)rbuf;
	req.aio_nbytes = 512;
	req.aio_offset = 0;
	
	reqs[0] = &req;
	
	err = syscall(__NR_io_submit, ctx, 1, &reqs);
	if (err < 0) {
		printf("io_submit err: %d\n", errno);
		return;
	} else if (err == 0) {
		printf("io_submit reports 0 successful IOCBs, but no error code. errno is %d", errno);
	}
	
	do {
		err = syscall(__NR_io_getevents, ctx, 0, 1, &evt, NULL);
		if (err < 0) {
			printf("io_getevents err: %d\n", errno);
			return;
		}
	} while (err < 1);
	
	printf("io_getevents success: %d\n", err);
	printf("evt.data: 0x%llX\n", evt.data);
	
	if (evt.obj == (__u64)&req) {
		printf("evt.obj matches &req\n");
	} else {
		printf("evt.obj does NOT match &req, 0x%llX given\n", evt.obj);
		printf("(&req is 0x%llX)\n", (__u64)&req);
	}
	
	printf("evt.res:  %lld\n", evt.res);
	printf("evt.res2: %lld\n", evt.res2);
	
	if (evt.res < 513) {
		rbuf[evt.res] = 0;
		printf("rbuf: %s\n", rbuf);
	} else {
		printf("rbuf: <exceeds buffer size>\n");
	}
	
	err = close(fd);
	if (err < 0) {
		printf("close err: %d\n", err);
		return;
	}
	
	printf("close success\n");
	
	err = syscall(__NR_io_destroy, ctx);
	if (err < 0) {
		printf("io_destroy err: %d\n", errno);
		return;
	}
	
	printf("io_destroy success\n");
}