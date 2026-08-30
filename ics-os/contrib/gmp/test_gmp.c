/* Host-side functional test of libgmp.a. gmp.h maps the short public names
 * (mpz_add, mpn_mul_1, ...) to the __gmp-prefixed symbols that the archive
 * defines, so this both links and runs against the freestanding objects.
 * Built/run on the host (make test-gmp); exercises mpz/mpf/mpq/rand plus the
 * internal mpn_* routines cc1 depends on. */
#include <stdio.h>
#include <stdlib.h>
#include "gmp.h"

static void check (const char *what, int ok)
{
  printf ("%s: %s\n", what, ok ? "OK" : "FAIL");
  if (!ok) exit (1);
}

int
main (void)
{
  mpz_t a, b, c, d;
  mpf_t f;
  mpq_t q;
  gmp_randstate_t rs;
  mp_limb_t limbs[8];
  mp_limb_t carry;

  mpz_init (a); mpz_init (b); mpz_init (c); mpz_init (d);
  mpf_init (f);
  mpq_init (q);

  mpz_set_ui (a, 123456789);
  mpz_set_ui (b, 987654321);

  mpz_add (c, a, b);
  check ("mpz_add 123456789+987654321", mpz_cmp_ui (c, 1111111110) == 0);

  mpz_mul (c, a, b);
  check ("mpz_mul", mpz_cmp_ui (c, 123456789ULL * 987654321ULL) == 0);

  mpz_set_ui (d, 1000000007);
  mpz_powm (c, a, b, d);
  check ("mpz_powm nonzero", mpz_sgn (c) == 1);

  char buf[128];
  mpz_get_str (buf, 10, c);
  check ("mpz_get_str", buf[0] != '\0');

  mpf_set_ui (f, 42);
  mpf_add (f, f, f);
  check ("mpf_add 42+42==84", mpf_cmp_ui (f, 84) == 0);

  mpq_set_ui (q, 1, 2);
  mpq_add (q, q, q);
  check ("mpq_add 1/2+1/2==1", mpq_cmp_ui (q, 1, 1) == 0);

  gmp_randinit_mt (rs);
  check ("gmp_randinit_mt", 1);
  gmp_randclear (rs);

  /* Internal mpn routines, referenced through the gmp.h -> __gmpn_* mapping. */
  limbs[0] = 5; limbs[1] = 7;
  carry = mpn_add_1 (limbs, limbs, 2, 3);
  check ("mpn_add_1 [5,7]+3", limbs[0] == 8 && limbs[1] == 7 && carry == 0);

  limbs[0] = 6; limbs[1] = 4;
  carry = mpn_mul_1 (limbs, limbs, 2, 7);
  check ("mpn_mul_1 [6,4]*7", limbs[0] == 42 && limbs[1] == 28 && carry == 0);

  mpz_clear (a); mpz_clear (b); mpz_clear (c); mpz_clear (d);
  mpf_clear (f);
  mpq_clear (q);

  printf ("GMP_HOST_TEST_PASS\n");
  return 0;
}
