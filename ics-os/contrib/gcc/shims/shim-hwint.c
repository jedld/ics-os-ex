/* Shim providing external definitions of the hwint log/scan helpers that
   the cc1 objects reference as external symbols. These are normally
   `static inline` in hwint.h when GCC_VERSION >= 3004, but the prebuilt
   objects here were compiled expecting out-of-line symbols. We provide the
   out-of-line definitions here.
   Self-contained: on this x86-64 host HOST_WIDE_INT is `long` (64-bit). */

#include <stddef.h>
#include <limits.h>

#ifndef HOST_WIDE_INT
#define HOST_WIDE_INT long
#endif
#ifndef HOST_BITS_PER_WIDE_INT
#define HOST_BITS_PER_WIDE_INT (sizeof (long) * CHAR_BIT)
#endif

/* Given X, an unsigned number, return the largest int Y such that 2**Y <= X.
   If X is 0, return -1.  */
int
floor_log2 (unsigned HOST_WIDE_INT x)
{
  int t = 0;
  if (x == 0)
    return -1;
  if (HOST_BITS_PER_WIDE_INT > 64)
    if (x >= (unsigned HOST_WIDE_INT) 1 << (t + 64))
      t += 64;
  if (HOST_BITS_PER_WIDE_INT > 32)
    if (x >= ((unsigned HOST_WIDE_INT) 1) << (t + 32))
      t += 32;
  if (x >= ((unsigned HOST_WIDE_INT) 1) << (t + 16))
    t += 16;
  if (x >= ((unsigned HOST_WIDE_INT) 1) << (t + 8))
    t += 8;
  if (x >= ((unsigned HOST_WIDE_INT) 1) << (t + 4))
    t += 4;
  if (x >= ((unsigned HOST_WIDE_INT) 1) << (t + 2))
    t += 2;
  if (x >= ((unsigned HOST_WIDE_INT) 1) << (t + 1))
    t += 1;
  return t;
}

/* Return the logarithm of X, base 2, if X is a power of 2.  Otherwise -1. */
int
exact_log2 (unsigned HOST_WIDE_INT x)
{
  if (x != (x & -x))
    return -1;
  return floor_log2 (x);
}

/* Number of least significant zero bits. X == 0 -> word size. */
int
ctz_hwi (unsigned HOST_WIDE_INT x)
{
  return x ? floor_log2 (x & -x) : HOST_BITS_PER_WIDE_INT;
}

/* Number of most significant zero bits. */
int
clz_hwi (unsigned HOST_WIDE_INT x)
{
  return HOST_BITS_PER_WIDE_INT - 1 - floor_log2 (x);
}

/* Like ctz_hwi but least significant bit numbered from 1; X == 0 -> 0. */
int
ffs_hwi (unsigned HOST_WIDE_INT x)
{
  return 1 + floor_log2 (x & -x);
}
