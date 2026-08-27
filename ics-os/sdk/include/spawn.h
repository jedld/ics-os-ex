#ifndef _SPAWN_H
#define _SPAWN_H

#include <sys/types.h>

typedef struct {
   int unused;
} posix_spawn_file_actions_t;

typedef struct {
   int unused;
} posix_spawnattr_t;

int posix_spawn(pid_t *pid, const char *path,
                const posix_spawn_file_actions_t *file_actions,
                const posix_spawnattr_t *attrp,
                char *const argv[], char *const envp[]);

#endif
