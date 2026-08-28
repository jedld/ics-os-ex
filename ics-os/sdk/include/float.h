#ifndef _FLOAT_H
#define _FLOAT_H

/* IEEE-754 float limits (x86-64). C99 <float.h>. Used by binutils
   libiberty floatformat.c (FLT_MAX, DBL_MAX, etc.). */

#define FLT_RADIX   2
#define FLT_MANT_DIG 24
#define DBL_MANT_DIG 53
#define LDBL_MANT_DIG 64

#define DBL_DIG     15
#define FLT_DIG     6
#define LDBL_DIG    18

#define DBL_EPSILON 2.2204460492503131e-016
#define FLT_EPSILON 1.192092896e-07
#define LDBL_EPSILON 1.0842021724855044e-019

#define DBL_MIN     2.2250738585072014e-308
#define FLT_MIN     1.175494351e-38
#define LDBL_MIN    3.3621031431120935e-4932

#define DBL_MAX     1.7976931348623157e+308
#define FLT_MAX     3.402823466e+38
#define LDBL_MAX    1.1897314953572318e+4932

#define DBL_MIN10   308
#define FLT_MIN10   38
#define LDBL_MIN10  4932

#define DBL_MAX10   308
#define FLT_MAX10   38
#define LDBL_MAX10  4932

#define FLT_QUIET_NAN  __builtin_nanf("0")
#define FLT_INFINITE   __builtin_huge_valf()
#define DBL_QUIET_NAN  __builtin_nan("0")
#define DBL_INFINITE   __builtin_huge_val()
#define LDBL_QUIET_NAN __builtin_nanl("0")
#define LDBL_INFINITE  __builtin_huge_vall()

#endif
