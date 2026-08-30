/* config.h for the MPC 1.0.1 freestanding SDK build (contrib/mpc).
 *
 * mpc-impl.h does an unguarded #include "config.h", so we ship the subset of
 * the autotools HAVE_* macros the ICS-OS SDK actually satisfies. Optional
 * features the SDK lacks (complex.h, locale.h, dlfcn.h) are deliberately left
 * undefined so MPC's #ifdef guards exclude them. BITS_PER_MP_LIMB is left
 * undefined so mpc-impl.h defaults it to GMP's mp_bits_per_limb.
 */

#define HAVE_STDLIB_H 1
#define HAVE_STDINT_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_INTMAX_T 1
#define HAVE_STRING_H 1
#define HAVE_STRINGS_H 1
#define HAVE_LIMITS_H 1
#define STDC_HEADERS 1

#define SIZEOF_INT 4
#define SIZEOF_LONG 8
#define SIZEOF_LONG_LONG 8
#define SIZEOF_SHORT 2
#define SIZEOF_POINTER 8
