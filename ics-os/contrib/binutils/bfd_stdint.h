/* ICS-OS: replaces the configure-generated bfd_stdint.h.
   Provides the C99 fixed-width integer types BFD expects on x86-64
   (long == 64-bit). Self-contained so it works under -nostdinc. */
#ifndef BFD_STDINT_H
#define BFD_STDINT_H

typedef signed char          bfd_int8_t;
typedef short int            bfd_int16_t;
typedef int                  bfd_int32_t;
typedef long int             bfd_int64_t;
typedef unsigned char        bfd_uint8_t;
typedef unsigned short int   bfd_uint16_t;
typedef unsigned int         bfd_uint32_t;
typedef unsigned long int    bfd_uint64_t;

typedef long int             int8_t;
typedef short int            int16_t;
typedef int                  int32_t;
typedef long int             int64_t;
typedef unsigned char        uint8_t;
typedef unsigned short int   uint16_t;
typedef unsigned int         uint32_t;
typedef unsigned long int    uint64_t;

typedef long int             intptr_t;
typedef unsigned long int    uintptr_t;
typedef long long int        intmax_t;
typedef unsigned long long   uintmax_t;

#define INT8_MAX   127
#define INT16_MAX  32767
#define INT32_MAX  2147483647
#define INT64_MAX  9223372036854775807L
#define UINT8_MAX  255
#define UINT16_MAX 65535
#define UINT32_MAX 4294967295UL
#define UINT64_MAX 18446744073709551615UL

#define INT8_C(x)   x
#define INT16_C(x)  x
#define INT32_C(x)  x
#define INT64_C(x)  x ## L
#define UINT8_C(x)  x
#define UINT16_C(x) x
#define UINT32_C(x) x ## U
#define UINT64_C(x) x ## UL

#endif /* BFD_STDINT_H */
