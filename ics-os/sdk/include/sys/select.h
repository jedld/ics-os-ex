#ifndef _SYS_SELECT_H
#define _SYS_SELECT_H

/*
 * select(2) for ICS-OS.
 *
 * NOTE: the fd_set word size (16 longs = 1024 fds) must match the kernel's
 * K_FD_SETSIZE in kernel/console/tty_tc.c. A struct timeval is passed in;
 * the kernel interprets tv_sec*1000 + tv_usec/1000 as a millisecond deadline
 * from getprecisetime().
 */

#include <sys/time.h>

#define FD_SETSIZE 1024

typedef struct {
    long fds_bits[16];
} fd_set;

#define FD_SET(n, p)   ((p)->fds_bits[(n) / 64] |= (1L << ((n) % 64)))
#define FD_CLR(n, p)   ((p)->fds_bits[(n) / 64] &= ~(1L << ((n) % 64)))
#define FD_ISSET(n, p) ((p)->fds_bits[(n) / 64] & (1L << ((n) % 64)))
#define FD_ZERO(p)     do { int _i; fd_set * _p = (p); \
                            for (_i = 0; _i < 16; _i++) _p->fds_bits[_i] = 0; \
                          } while (0)

int select(int nfds, fd_set *readfds, fd_set *writefds,
           fd_set *exceptfds, struct timeval *timeout);

#endif
