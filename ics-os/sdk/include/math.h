#ifndef _MATH_H
#define _MATH_H

#define HUGE_VAL (1.0e300)

double strtod(const char *nptr, char **endptr);
float strtof(const char *nptr, char **endptr);
long double strtold(const char *nptr, char **endptr);
double fabs(double x);
double floor(double x);
double ceil(double x);
double ldexp(double x, int exp);

#endif
