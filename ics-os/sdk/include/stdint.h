#ifndef _STDINT_H
#define _STDINT_H

typedef signed char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long long int64_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

#ifdef __x86_64__
typedef long intptr_t;
typedef unsigned long uintptr_t;
#else
typedef int intptr_t;
typedef unsigned int uintptr_t;
#endif

#define INT32_MAX 2147483647
#define UINT32_MAX 4294967295U

/* 64-bit limits + C99 intmax_t/uintmax_t. Required by C99 and used by
   binutils/libiberty (strtoumax, PRIxMAX, etc.). On x86-64 `long` is 64-bit. */
#ifdef __x86_64__
typedef long intmax_t;
typedef unsigned long uintmax_t;
#else
typedef long long intmax_t;
typedef unsigned long long uintmax_t;
#endif

#define INT64_MAX 9223372036854775807LL
#define INT64_MIN (-9223372036854775807LL - 1)
#define UINT64_MAX 18446744073709551615ULL
#define INTMAX_MAX INT64_MAX
#define INTMAX_MIN INT64_MIN
#define UINTMAX_MAX UINT64_MAX

#endif
