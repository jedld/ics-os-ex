#ifndef _SIGNAL_H
#define _SIGNAL_H

#include <stddef.h>

/* Smallest type in which a signal handler may store an int and reference it
   atomically (C/POSIX). ICS-OS delivers no async signals in ring 3, so a
   plain int is sufficient for the mask bookkeeping vim and the SDK do. */
typedef int sig_atomic_t;

typedef void (*sighandler_t)(int);
#define SIG_DFL ((sighandler_t)0)
#define SIG_IGN ((sighandler_t)1)
/* Standard signal numbers (Linux x86-64). ICS-OS does not deliver most of
   these asynchronously in ring 3, but the constants are defined so that
   portable code (vim's os_unix.c, the binutils/ld job-control paths) that
   branches on them compiles and its handlers are simply never invoked. */
#define SIGHUP  1
#define SIGINT  2
#define SIGQUIT 3
#define SIGILL  4
#define SIGTRAP 5
#define SIGABRT 6
#define SIGBUS  7
#define SIGFPE  8
#define SIGKILL 9
#define SIGUSR1 10
#define SIGSEGV 11
#define SIGUSR2 12
#define SIGPIPE 13
#define SIGALRM 14
#define SIGTERM 15
#define SIGSTKFLT 16
#define SIGCHLD 17
#define SIGCONT 18
#define SIGSTOP 19
#define SIGTSTP 20
#define SIGTTIN 21
#define SIGTTOU 22
#define SIGURG  23
#define SIGXCPU 24
#define SIGXFSZ 25
#define SIGVTALRM 26
#define SIGPROF 27
#define SIGWINCH 28
#define SIGIO   29
#define SIGPWR  30
#define SIGSYS  31
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
