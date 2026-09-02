#ifndef _SYS_POLL_H
#define _SYS_POLL_H

/* poll(2) for ICS-OS. Implemented on top of select() in posix.c. */

#define POLLIN   0x001
#define POLLPRI  0x002
#define POLLOUT  0x004
#define POLLERR  0x008
#define POLLHUP  0x010
#define POLLNVAL 0x020

struct pollfd {
    int fd;
    short events;
    short revents;
};

int poll(struct pollfd *fds, unsigned int nfds, int timeout);

#endif
