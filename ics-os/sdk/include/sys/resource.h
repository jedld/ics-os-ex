#ifndef _SYS_RESOURCE_H
#define _SYS_RESOURCE_H

/* <sys/resource.h> for ICS-OS. Minimal resource-limit support: the
   RLIMIT_* resources used by GNU binutils (libiberty stack-limit.c:
   RLIMIT_STACK, RLIMIT_AS, RLIMIT_DATA, RLIMIT_FSIZE, RLIMIT_NOFILE) and a
   struct rlimit. getrlimit/setrlimit are implemented in posix.c. */

#include <sys/types.h>
#include <stddef.h>

#ifndef __RLIM_T
#define __RLIM_T
typedef unsigned long rlim_t;
#endif

struct rlimit {
   rlim_t rlim_cur;
   rlim_t rlim_max;
};

#define RLIM_INFINITY  ((rlim_t)-1)
#define RLIM_SAVED_CUR ((rlim_t)-2)
#define RLIM_SAVED_MAX ((rlim_t)-3)

#define RLIMIT_CPU    0
#define RLIMIT_FSIZE  1
#define RLIMIT_DATA   2
#define RLIMIT_STACK  3
#define RLIMIT_CORE   4
#define RLIMIT_RSS    5
#define RLIMIT_NPROC  6
#define RLIMIT_NOFILE 7
#define RLIMIT_MEMLOCK 8
#define RLIMIT_AS     9

int getrlimit(int resource, struct rlimit *rlim);
int setrlimit(int resource, const struct rlimit *rlim);

#endif
