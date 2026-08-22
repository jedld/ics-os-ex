#ifndef _STDDEF_H
#define _STDDEF_H

#ifndef NULL
#define NULL ((void*)0)
#endif

#ifdef __x86_64__
typedef unsigned long size_t;
typedef long ptrdiff_t;
typedef long ssize_t;
typedef unsigned long uintptr_t;
typedef long intptr_t;
#else
typedef unsigned int size_t;
typedef int ptrdiff_t;
typedef int ssize_t;
typedef unsigned int uintptr_t;
typedef int intptr_t;
#endif
typedef int wchar_t;

#ifndef offsetof
#define offsetof(type, field) ((size_t)&((type *)0)->field)
#endif

#endif
