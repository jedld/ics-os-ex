#ifndef _ERRNO_H
#define _ERRNO_H

extern int errno;

#define EPERM   1
#define ENOENT  2
#define EINTR   4
#define EIO     5
#define EBADF   9
#define ENOMEM  12
#define EACCES  13
#define EEXIST  17
#define ENOTDIR 20
#define EINVAL  22
#define EMFILE  24
#define ENOSPC  28
#define ERANGE  34
#define ENOSYS  38
#define ENOEXEC 8
#define ECHILD  10
#define ENAMETOOLONG 36
#define ELOOP     40
#define EISDIR    21
#define ENOTEMPTY 39
#define EPIPE     32
#define ESRCH     3
#define EDEADLK   35

#endif
