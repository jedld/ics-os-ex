#ifndef _FCNTL_H
#define _FCNTL_H

#define O_RDONLY        0x0000
#define O_WRONLY        0x0001
#define O_RDWR          0x0002
#define O_ACCMODE       0x0003
#define O_BINARY        0x0004
#define O_TEXT          0x0008
#define O_CREAT         0x0100
#define O_EXCL          0x0200
#define O_TRUNC         0x0800
#define O_APPEND        0x1000
#define O_NONBLOCK      0x2000
#define O_DIRECT        0x4000
#define O_SYNC          0x10000
#define O_NOCTTY        0x80
#define O_DIRECTORY     0x1000000

/* fcntl(2) commands and FD_CLOEXEC (values are ICS-OS-internal, not the
   Linux numbers: the kernel's fcntl switch matches these). */
#define F_DUPFD         0
#define F_GETFD         1
#define F_SETFD         2
#define F_GETFL         3
#define F_SETFL         4
#define F_GETLK         5
#define F_SETLK         6
#define F_SETLKW        7

#define FD_CLOEXEC      1

int open(const char *path, int flags, ...);
int creat(const char *path, int mode);
int fcntl(int fd, int cmd, ...);

#endif
