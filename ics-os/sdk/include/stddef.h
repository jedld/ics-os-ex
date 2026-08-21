#ifndef _STDDEF_H
#define _STDDEF_H

#ifndef NULL
#define NULL ((void*)0)
#endif

typedef unsigned int size_t;
typedef int ptrdiff_t;
typedef int ssize_t;
typedef unsigned int uintptr_t;
typedef int intptr_t;
typedef int wchar_t;

#ifndef offsetof
#define offsetof(type, field) ((size_t)&((type *)0)->field)
#endif

#endif
