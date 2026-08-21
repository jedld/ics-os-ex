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

int open(const char *path, int flags, ...);
int creat(const char *path, int mode);

#endif
