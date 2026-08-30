/* Host functional test for libmpfr.a (MPFR 3.0.1, built via the ICS-OS SDK).
 *
 * MPFR's public API requires <gmp.h> to be included BEFORE <mpfr.h> (mpfr.h
 * uses mpz_t/mpf_t types but does not include gmp.h). We link our own
 * libmpfr.a + libgmp.a, so both headers resolve to the overlay gmp.h mapping
 * (GMP symbols carry the double-underscore prefix) plus the MPFR headers.
 *
 * Exercises: init/prec, set/get of double and int, the four arithmetic ops,
 * sqrt, pow, exp, log, sin, cos, and comparisons. MPFR implements these
 * transcendentals itself (no host libm), so this also smoke-tests the
 * multiple-precision paths against known values.
 */
#include <stdio.h>
#include <gmp.h>
#include <mpfr.h>

#define PREC 53   /* double-precision significand */

static int fails;

static void check_d(const char *name, mpfr_t x, double expect, double eps)
{
  double got = mpfr_get_d(x, MPFR_RNDN);
  double diff = got - expect;
  if (diff < 0) diff = -diff;
  if (diff > eps) {
    printf("FAIL %s: got %g want %g\n", name, got, expect);
    fails++;
  } else {
    printf("ok   %s = %g\n", name, got);
  }
}

int main(void)
{
  mpfr_t a, b, c;
  mpz_t  z;

  mpfr_init2(a, PREC);
  mpfr_init2(b, PREC);
  mpfr_init2(c, PREC);
  mpz_init(z);

  /* set from double / int */
  mpfr_set_d(a, 3.0, MPFR_RNDN);
  mpfr_set_d(b, 4.0, MPFR_RNDN);
  check_d("3.0", a, 3.0, 1e-9);

  /* arithmetic: 3+4=7, 3*4=12, 4/3, 4-3=1 */
  mpfr_add(c, a, b, MPFR_RNDN);      check_d("3+4",  c, 7.0, 1e-9);
  mpfr_mul(c, a, b, MPFR_RNDN);      check_d("3*4",  c, 12.0, 1e-9);
  mpfr_sub(c, b, a, MPFR_RNDN);      check_d("4-3",  c, 1.0, 1e-9);
  mpfr_div(c, b, a, MPFR_RNDN);      check_d("4/3",  c, 4.0/3.0, 1e-9);

  /* sqrt(2) */
  mpfr_set_d(c, 2.0, MPFR_RNDN);
  mpfr_sqrt(c, c, MPFR_RNDN);        check_d("sqrt(2)", c, 1.4142135623730951, 1e-9);

  /* pow(2,10)=1024 exact */
  mpfr_set_d(a, 2.0, MPFR_RNDN);
  mpfr_set_d(b, 10.0, MPFR_RNDN);
  mpfr_pow(c, a, b, MPFR_RNDN);      check_d("2^10", c, 1024.0, 1e-9);

  /* exp(1)=e, log(10) */
  mpfr_set_d(a, 1.0, MPFR_RNDN);
  mpfr_exp(c, a, MPFR_RNDN);         check_d("e", c, 2.7182818284590452, 1e-9);
  mpfr_set_d(a, 10.0, MPFR_RNDN);
  mpfr_log(c, a, MPFR_RNDN);         check_d("log(10)", c, 2.3025850929940457, 1e-9);

  /* sin(pi/2)=1, cos(0)=1 */
  {
    double pi = 3.14159265358979323846;
    mpfr_t half;
    mpfr_init2(half, PREC);
    mpfr_set_d(half, pi/2.0, MPFR_RNDN);
    mpfr_sin(c, half, MPFR_RNDN);    check_d("sin(pi/2)", c, 1.0, 1e-9);
    mpfr_set_d(half, 0.0, MPFR_RNDN);
    mpfr_cos(c, half, MPFR_RNDN);    check_d("cos(0)", c, 1.0, 1e-9);
    mpfr_clear(half);
  }

  /* comparisons + round-trip to GMP */
  mpfr_set_d(a, 1.0, MPFR_RNDN);
  mpfr_set_d(b, 10.0, MPFR_RNDN);
  if (mpfr_cmp(a, b) == 0) { printf("FAIL cmp 1.0==10.0\n"); fails++; }
  if (mpfr_cmp(b, a) <= 0) { printf("FAIL cmp 10.0<=1.0\n"); fails++; }
  mpfr_set_d(a, 42.0, MPFR_RNDN);
  mpfr_get_z(z, a, MPFR_RNDN);
  if (mpz_get_si(z) != 42) { printf("FAIL get_z\n"); fails++; }

  /* version string sanity */
  printf("mpfr %s\n", mpfr_get_version());

  mpfr_clears(a, b, c, NULL);
  mpz_clear(z);

  if (fails) { printf("MPFR_HOST_TEST_FAIL (%d)\n", fails); return 1; }
  printf("MPFR_HOST_TEST_PASS\n");
  return 0;
}
