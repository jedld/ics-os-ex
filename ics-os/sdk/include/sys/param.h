#ifndef _SYS_PARAM_H
#define _SYS_PARAM_H

/* <sys/param.h> for ICS-OS. Provides the small set of macros GNU binutils
   (libiberty + libbfd) expect: PATH_MAX, MAXPATHLEN, PAGE_SIZE, and the
   MIN/MAX helpers. Values match x86-64 long mode (4 KiB pages). */

#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#ifndef MAXPATHLEN
#define MAXPATHLEN PATH_MAX
#endif

#ifndef NAME_MAX
#define NAME_MAX 255
#endif

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

#ifndef PAGE_SHIFT
#define PAGE_SHIFT 12
#endif

#ifndef MB
#define MB (1024 * 1024)
#endif

#ifndef KB
#define KB 1024
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef howmany
#define howmany(x, y) ((((x)-1)/(y)) + 1)
#endif

#ifndef roundup
#define roundup(x, y) ((((x)+((y)-1))/(y))*(y))
#endif

#ifndef powerof2
#define powerof2(x) ((((x)-1) & (x)) == 0)
#endif

#endif
