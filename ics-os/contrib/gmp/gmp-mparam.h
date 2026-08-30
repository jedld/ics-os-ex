/*
 * GMP 5.1.3 gmp-mparam.h — minimal stand-in for the configure-generated
 * header. The generated gmp.h already sets GMP_LIMB_BITS=64 / GMP_NAIL_BITS=0
 * and mp_limb_t=unsigned long long; gmp-impl.h derives BYTES_PER_MP_LIMB from
 * SIZEOF_MP_LIMB_T. These two are provided here (guarded) to be explicit for a
 * 64-bit-limb x86-64 build.
 */
#ifndef ICSOS_GMP_MPARAM_H
#define ICSOS_GMP_MPARAM_H
#ifndef BYTES_PER_MP_LIMB
#define BYTES_PER_MP_LIMB 8
#endif
#ifndef BITS_PER_MP_LIMB
#define BITS_PER_MP_LIMB 64
#endif
#endif
