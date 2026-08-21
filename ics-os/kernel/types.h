/*
 * Common scalar and address types for ICS-OS (32-bit and LP64).
 * On-disk / ABI fixed-width fields should use u32/u16/u8, not pointer-sized types.
 */
#ifndef ICSOS_TYPES_H
#define ICSOS_TYPES_H

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

typedef signed char        s8;
typedef short              s16;
typedef int                s32;
typedef long long          s64;

#ifdef __x86_64__
typedef unsigned long      uintptr;
typedef unsigned long      size_t_k;
typedef unsigned long      vaddr_t;
typedef unsigned long      paddr_t;
#else
typedef unsigned int       uintptr;
typedef unsigned int       size_t_k;
typedef unsigned int       vaddr_t;
typedef unsigned int       paddr_t;
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

#endif /* ICSOS_TYPES_H */
