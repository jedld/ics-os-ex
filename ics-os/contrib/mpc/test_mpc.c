#include <stdio.h>
#include <stdlib.h>
#include <gmp.h>
#include <mpfr.h>
#include <mpc.h>
int main(void){
  mpc_t a,b,c;
  mpc_init2(a,64); mpc_init2(b,64); mpc_init2(c,64);
  mpfr_set_d(mpc_realref(a),3,GMP_RNDN); mpfr_set_d(mpc_imagref(a),4,GMP_RNDN);
  mpfr_set_d(mpc_realref(b),1,GMP_RNDN); mpfr_set_d(mpc_imagref(b),2,GMP_RNDN);
  mpc_mul(c,a,b,GMP_RNDN);
  char *s=mpc_get_str(10,20,c,GMP_RNDN);
  printf("mul: %s (expect -5+10i)\n",s); free(s);
  mpc_sqrt(c,a,GMP_RNDN);
  s=mpc_get_str(10,20,c,GMP_RNDN);
  printf("sqrt: %s (expect 2+1i)\n",s); free(s);
  printf("MPC_TEST_PASS\n");
  mpc_clear(a);mpc_clear(b);mpc_clear(c);
  return 0;
}
