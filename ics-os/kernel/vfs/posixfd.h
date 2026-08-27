#ifndef ICSOS_POSIXFD_H
#define ICSOS_POSIXFD_H

#include "../types.h"

#define ICSOS_O_RDONLY   0x0000
#define ICSOS_O_WRONLY   0x0001
#define ICSOS_O_RDWR     0x0002
#define ICSOS_O_ACCMODE  0x0003
#define ICSOS_O_CREAT    0x0100
#define ICSOS_O_TRUNC    0x0800
#define ICSOS_O_APPEND   0x1000
#define ICSOS_O_DIRECT   0x4000

#define ICSOS_AT_FDCWD   (-100)

#define IORING_OP_NOP     0
#define IORING_OP_READV   1
#define IORING_OP_WRITEV  2
#define IORING_OP_FSYNC   3
#define IORING_OP_OPENAT  18
#define IORING_OP_CLOSE   19
#define IORING_OP_READ    22
#define IORING_OP_WRITE   23

#define IORING_ENTER_GETEVENTS 1

struct k_iovec {
   void *iov_base;
   unsigned long iov_len;
};

int sys_open(const char *path, int flags, int mode);
int sys_close(int fd);
long sys_read(int fd, void *buf, long n);
long sys_write(int fd, const void *buf, long n);
long sys_lseek(int fd, long off, int whence);
long sys_preadv(int fd, const struct k_iovec *iov, int iovcnt, long offset);
long sys_pwritev(int fd, const struct k_iovec *iov, int iovcnt, long offset);
int  sys_fsync(int fd);
int  sys_fstat_fd(int fd, void *statbuf);
int  sys_io_uring_setup(unsigned entries, void *params);
int  sys_io_uring_enter(int fd, unsigned to_submit, unsigned min_complete,
                        unsigned flags);
void *sys_fd_file(int fd);

int sys_waitpid(int pid, int *status, int options);
int sys_spawn(const char *path, const char *params);
int sys_execve(const char *path, const char *params);
int sys_getdents(const char *path, char *buf, int buflen);

#endif
