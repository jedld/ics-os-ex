#ifndef _SIGNAL_H
#define _SIGNAL_H

typedef void (*sighandler_t)(int);
#define SIG_DFL ((sighandler_t)0)
#define SIG_IGN ((sighandler_t)1)
#define SIGINT  2
#define SIGCHLD 17
#define SIGSEGV 11
#define SIGTERM 15
#define SIGKILL 9
#define NSIG 32

#define SIG_BLOCK   0
#define SIG_UNBLOCK 1
#define SIG_SETMASK 2

sighandler_t signal(int sig, sighandler_t handler);
int kill(int pid, int sig);

#endif
