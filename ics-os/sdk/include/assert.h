#ifndef _ASSERT_H
#define _ASSERT_H

#include <stdlib.h> /* abort() */

#ifdef NDEBUG
#define assert(x) ((void)0)
#else
#define assert(x) do { if (!(x)) abort(); } while (0)
#endif

#endif
