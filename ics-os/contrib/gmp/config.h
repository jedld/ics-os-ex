/*
 * GMP 5.1.3 config.h — hand-written for an x86-64 host, built against the
 * ICS-OS SDK (no autotools). Provides the SIZEOF_ and HAVE_ macros that
 * gmp-impl.h and the sources expect. Limb params (64-bit long-long limbs)
 * come from the generated gmp.h (see gmp-h.in); gmp-impl.h falls back to
 * SIZEOF_MP_LIMB_T for BYTES_PER_MP_LIMB.
 */
#ifndef ICSOS_GMP_CONFIG_H
#define ICSOS_GMP_CONFIG_H

/* ---- host word sizes (x86-64 / LP64) ---- */
#define SIZEOF_UNSIGNED_SHORT     2
#define SIZEOF_UNSIGNED_INT       4
#define SIZEOF_UNSIGNED           4
#define SIZEOF_UNSIGNED_LONG      8
#define SIZEOF_UNSIGNED_LONG_LONG 8
#define SIZEOF_INT                4
#define SIZEOF_SHORT              2
#define SIZEOF_LONG               8
#define SIZEOF_LONG_LONG          8
#define SIZEOF_SIZE_T             8
#define SIZEOF_PTRDIFF_T          8
#define SIZEOF_MP_LIMB_T          8   /* 64-bit limb (unsigned long long) */

/* ---- integer types present in the SDK ---- */
#define HAVE_INT32_T            1
#define HAVE_UINT32_T           1
#define HAVE_INT64_T            1
#define HAVE_UINT64_T           1
#define HAVE_INTPTR_T           1
#define HAVE_UINTPTR_T          1
#define HAVE_UINT_LEAST32_T     1
#define HAVE_INTTYPES_H         1
#define HAVE_STDINT_H           1
#define HAVE_STDARG             1
#define HAVE_LONG_LONG          1

/* ---- floating point: IEEE-754, little-endian ---- */
#define HAVE_DOUBLE_IEEE_LITTLE_ENDIAN 1

/* ---- tmp storage: single-threaded, non-reentrant (tal-notreent.c) ---- */
#define WANT_TMP_NOTREENTRANT         1

/* ---- libc pieces the SDK provides ----
 * NOTE: no locale support in the SDK (no locale.h/localeconv/nl_langinfo), so
 * HAVE_LOCALECONV / HAVE_NL_LANGINFO are left undefined -> GMP_DECIMAL_POINT
 * falls back to "." (correct for a bootstrap compiler).
 */
#define HAVE_STDLIB_H           1
#define HAVE_STRING_H           1
#define HAVE_UNISTD_H           1
#define HAVE_MALLOC_H           1
/* SDK string.h provides these; defining them stops gmp-impl.h from emitting
   its do/while fallback macros (which clash with the SDK's memset decl). */
#define HAVE_MEMSET             1
#define HAVE_MEMMOVE            1
#define HAVE_STRCHR             1

/* ---- deliberately undefined (kept simple / portable) ----
 * HAVE_ATTRIBUTE_*   -> __GMP_ATTRIBUTE_* left empty (harmless).
 * HAVE_NL_LANGINFO is set; localeconv is a stub in the SDK.
 * No threads (GMP isn't built thread-safe here); no C++.
 */

#endif
