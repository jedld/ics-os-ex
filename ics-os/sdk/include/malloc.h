#ifndef _MALLOC_H
#define _MALLOC_H

/* Legacy <malloc.h> (glibc provides it as a thin wrapper over <stdlib.h>).
   GNU binutils libiberty (hashtab.c) still includes it. */

#include <stdlib.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

#endif
