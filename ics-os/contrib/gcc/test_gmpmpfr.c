/* Wiring regression test for the contrib/gcc overlay.
 *
 * Compiled with the SAME flags cc1 uses (contrib/gcc CFLAGS: -nostdinc, SDK
 * headers, -DHAVE_CONFIG_H, the GCC internal include dirs, and the
 * SDK-built GMP/MPFR overlay headers) and linked against the SDK .c files plus
 * libgmp.a + libmpfr.a. It fails at link time (-Wl,--no-undefined) if the
 * GMP/MPFR header-to-symbol wiring is wrong, e.g. if cc1 were pointed at the
 * host gmp.h (bare mpz_*) instead of our overlay gmp.h (__gmpz_*).
 */
#include <gmp.h>
#include <mpfr.h>

int main(void)
{
  /* GMP path: mpz_add must resolve to __gmpz_add in libgmp.a */
  mpz_t a, b;
  mpz_init(a);
  mpz_init(b);
  mpz_set_si(a, 40);
  mpz_set_ui(b, 2);
  mpz_add(a, a, b);              /* a = 42 */
  if (mpz_get_si(a) != 42) return 1;

  /* MPFR path: mpfr_* must resolve to bare mpfr_* in libmpfr.a */
  mpfr_t x, y;
  mpfr_init2(x, 53);
  mpfr_init2(y, 53);
  mpfr_set_d(x, 6.0, MPFR_RNDN);
  mpfr_set_d(y, 7.0, MPFR_RNDN);
  mpfr_mul(x, x, y, MPFR_RNDN);  /* x = 42.0 */
  if (mpfr_get_d(x, MPFR_RNDN) != 42.0) return 2;

  mpz_clear(a);
  mpz_clear(b);
  mpfr_clear(x);
  mpfr_clear(y);
  return 0;
}
