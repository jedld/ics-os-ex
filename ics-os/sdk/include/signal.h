#ifndef _SIGNAL_H
#define _SIGNAL_H

#include <stddef.h>

typedef void (*sighandler_t)(int);
#define SIG_DFL ((sighandler_t)0)
#define SIG_IGN ((sighandler_t)1)
#define SIGINT  2
#define SIGCHLD 17
#define SIGSEGV 11
#define SIGTERM 15
#define SIGKILL 9
#define SIGABRT 6
#define SIGALRM 14
#define SIGFPE  8
#define SIGPIPE 13
#define SIGUSR1 10
#define SIGUSR2 12
#define NSIG 32

#define SIG_BLOCK   0
#define SIG_UNBLOCK 1
#define SIG_SETMASK 2

/* Signal set (POSIX). ICS-OS has no per-process hardware signal delivery;
   the set is maintained in the SDK so signal-mask APIs are functional
   (binutils libiberty sigsetmask.c, and ld's job control). */
typedef struct {
   unsigned long bits[4];   /* 128 signals */
} sigset_t;

struct sigaction {
   sighandler_t sa_handler;
   unsigned long sa_flags;
   void (*sa_restorer)(void);
};

#define SA_RESTART 0x10000001
#define SA_SIGINFO 0x00000002

void sigemptyset(sigset_t *set);
void sigfillset(sigset_t *set);
void sigaddset(sigset_t *set, int signum);
void sigdelset(sigset_t *set, int signum);
int sigismember(const sigset_t *set, int signum);
int sigprocmask(int how, const sigset_t *set, sigset_t *oldset);
sighandler_t signal(int sig, sighandler_t handler);
int kill(int pid, int sig);
int raise(int sig);

#endif
