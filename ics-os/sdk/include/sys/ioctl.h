#ifndef _SYS_IOCTL_H
#define _SYS_IOCTL_H

/*
 * ioctl for ICS-OS.
 *
 * NOTE: struct winsize must stay byte-identical to the kernel copy in
 * kernel/console/tty_tc.c (4 x unsigned int).
 */

struct winsize {
    unsigned int ws_row;
    unsigned int ws_col;
    unsigned int ws_xpixel;
    unsigned int ws_ypixel;
};

#define TIOCGWINSZ 0x5413
#define TIOCSWINSZ 0x5414
#define TIOCGPGRP  0x541F
#define TIOCSPGRP  0x5420

int ioctl(int fd, unsigned long request, ...);

#endif
