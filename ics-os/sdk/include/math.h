#ifndef _MATH_H
#define _MATH_H

#define HUGE_VAL (1.0e300)

/* C99 floating-point constants and classification macros. The freestanding
   target has no libm, so isinf/isnan/isfinite are provided as macros (the
   standard's portable form) rather than linked functions. __builtin_huge_val()
   yields IEEE +inf and survives -fno-builtin (it is a frontend builtin). */
#define INFINITY (__builtin_huge_val())
#define NAN (__builtin_nanf(""))
#define isinf(x) ((x) == INFINITY || (x) == -INFINITY)
#define isnan(x) ((x) != (x))
#define isfinite(x) (isinf(x) == 0 && isnan(x) == 0)

double strtod(const char *nptr, char **endptr);
float strtof(const char *nptr, char **endptr);
long double strtold(const char *nptr, char **endptr);
double fabs(double x);
double floor(double x);
double ceil(double x);
double ldexp(double x, int exp);
double log10(double x);

#endif
