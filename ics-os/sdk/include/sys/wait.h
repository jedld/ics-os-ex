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

#endif
