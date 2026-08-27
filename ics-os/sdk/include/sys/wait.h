#ifndef _SYS_WAIT_H
#define _SYS_WAIT_H

#include <sys/types.h>

#define WNOHANG    1
#define WUNTRACED  2

#define WIFEXITED(st)    (1)
#define WEXITSTATUS(st)  ((int)(st) & 0xff)
#define WIFSIGNALED(st)  (0)
#define WTERMSIG(st)     (0)

pid_t waitpid(pid_t pid, int *status, int options);

/* POSIX wait: wait for any child to exit, return its pid and store the
 * status word. Delegates to waitpid(-1, status, 0) in the SDK. */
int wait(int *status);

#endif
