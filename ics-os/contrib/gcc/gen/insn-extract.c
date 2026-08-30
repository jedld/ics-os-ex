/* Generated automatically by the program `genextract'
   from the machine description file `md'.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "rtl.h"
#include "insn-config.h"
#include "recog.h"
#include "diagnostic-core.h"

/* This variable is used as the "location" of any missing operand
   whose numbers are skipped by a given pattern.  */
static rtx junk ATTRIBUTE_UNUSED;

void
insn_extract (rtx insn)
{
  rtx *ro = recog_data.operand;
  rtx **ro_loc = recog_data.operand_loc;
  rtx pat = PATTERN (insn);
  int i ATTRIBUTE_UNUSED; /* only for peepholes */

#ifdef ENABLE_CHECKING
  memset (ro, 0xab, sizeof (*ro) * MAX_RECOG_OPERANDS);
  memset (ro_loc, 0xab, sizeof (*ro_loc) * MAX_RECOG_OPERANDS);
#endif

  switch (INSN_CODE (insn))
    {
    default:
      /* Control reaches here if insn_extract has been called with an
         unrecognizable insn (code -1), or an insn whose INSN_CODE
         corresponds to a DEFINE_EXPAND in the machine description;
         either way, a bug.  */
      if (INSN_CODE (insn) < 0)
        fatal_insn ("unrecognizable insn:", insn);
      else
        fatal_insn ("insn with invalid code number:", insn);

    case 2057:  /* atomic_xordi */
    case 2056:  /* atomic_ordi */
    case 2055:  /* atomic_anddi */
    case 2054:  /* atomic_xorsi */
    case 2053:  /* atomic_orsi */
    case 2052:  /* atomic_andsi */
    case 2051:  /* atomic_xorhi */
    case 2050:  /* atomic_orhi */
    case 2049:  /* atomic_andhi */
    case 2048:  /* atomic_xorqi */
    case 2047:  /* atomic_orqi */
    case 2046:  /* atomic_andqi */
    case 2045:  /* atomic_subdi */
    case 2044:  /* atomic_subsi */
    case 2043:  /* atomic_subhi */
    case 2042:  /* atomic_subqi */
    case 2041:  /* atomic_adddi */
    case 2040:  /* atomic_addsi */
    case 2039:  /* atomic_addhi */
    case 2038:  /* atomic_addqi */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 0), 1));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 1));
      recog_data.dup_loc[0] = &XEXP (XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 0), 0);
      recog_data.dup_num[0] = 0;
      break;

    case 2037:  /* atomic_exchangedi */
    case 2036:  /* atomic_exchangesi */
    case 2035:  /* atomic_exchangehi */
    case 2034:  /* atomic_exchangeqi */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 0));
      ro[2] = *(ro_loc[2] = &XEXP (XVECEXP (pat, 0, 1), 1));
      ro[3] = *(ro_loc[3] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 1));
      recog_data.dup_loc[0] = &XEXP (XVECEXP (pat, 0, 1), 0);
      recog_data.dup_num[0] = 1;
      break;

    case 2033:  /* *atomic_fetch_add_cmpdi */
    case 2032:  /* *atomic_fetch_add_cmpsi */
    case 2031:  /* *atomic_fetch_add_cmphi */
    case 2030:  /* *atomic_fetch_add_cmpqi */
      ro[0] = *(ro_loc[0] = &XVECEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 1));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1));
      ro[3] = *(ro_loc[3] = &XVECEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0, 1));
      recog_data.dup_loc[0] = &XEXP (XVECEXP (pat, 0, 1), 0);
      recog_data.dup_num[0] = 0;
      recog_data.dup_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0);
      recog_data.dup_num[1] = 0;
      break;

    case 2029:  /* atomic_fetch_adddi */
    case 2028:  /* atomic_fetch_addsi */
    case 2027:  /* atomic_fetch_addhi */
    case 2026:  /* atomic_fetch_addqi */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 1));
      ro[3] = *(ro_loc[3] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 1));
      recog_data.dup_loc[0] = &XEXP (XVECEXP (pat, 0, 1), 0);
      recog_data.dup_num[0] = 1;
      recog_data.dup_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0);
      recog_data.dup_num[1] = 1;
      break;

    case 2025:  /* atomic_compare_and_swapti_doubleword */
    case 2024:  /* atomic_compare_and_swapdi_doubleword */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 0));
      ro[3] = *(ro_loc[3] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 1));
      ro[4] = *(ro_loc[4] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 2));
      ro[5] = *(ro_loc[5] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 3));
      ro[6] = *(ro_loc[6] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 4));
      ro[7] = *(ro_loc[7] = &XEXP (XVECEXP (pat, 0, 4), 0));
      recog_data.dup_loc[0] = &XEXP (XVECEXP (pat, 0, 2), 0);
      recog_data.dup_num[0] = 2;
      break;

    case 2023:  /* atomic_compare_and_swapdi_1 */
    case 2022:  /* atomic_compare_and_swapsi_1 */
    case 2021:  /* atomic_compare_and_swaphi_1 */
    case 2020:  /* atomic_compare_and_swapqi_1 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 1));
      ro[3] = *(ro_loc[3] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 2));
      recog_data.dup_loc[0] = &XEXP (XVECEXP (pat, 0, 1), 0);
      recog_data.dup_num[0] = 1;
      break;

    case 2011:  /* mfence_nosse */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      recog_data.dup_loc[0] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 0);
      recog_data.dup_num[0] = 0;
      break;

    case 2007:  /* *avx2_gatherdiv8sf_4 */
    case 2006:  /* *avx2_gatherdiv8sf_4 */
    case 2005:  /* *avx2_gatherdiv8si_4 */
    case 2004:  /* *avx2_gatherdiv8si_4 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (XVECEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0, 1), 0), 0, 0));
      ro[3] = *(ro_loc[3] = &XVECEXP (XEXP (XVECEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0, 1), 0), 0, 1));
      ro[4] = *(ro_loc[4] = &XVECEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0, 3));
      ro[5] = *(ro_loc[5] = &XVECEXP (XEXP (XVECEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0, 1), 0), 0, 2));
      ro[6] = *(ro_loc[6] = &XVECEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0, 1));
      break;

    case 2003:  /* *avx2_gatherdiv8sf_3 */
    case 2002:  /* *avx2_gatherdiv8sf_3 */
    case 2001:  /* *avx2_gatherdiv8si_3 */
    case 2000:  /* *avx2_gatherdiv8si_3 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0, 0));
      ro[3] = *(ro_loc[3] = &XVECEXP (XEXP (XVECEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0, 1), 0), 0, 0));
      ro[4] = *(ro_loc[4] = &XVECEXP (XEXP (XVECEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0, 1), 0), 0, 1));
      ro[5] = *(ro_loc[5] = &XVECEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0, 3));
      ro[6] = *(ro_loc[6] = &XVECEXP (XEXP (XVECEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0, 1), 0), 0, 2));
      ro[7] = *(ro_loc[7] = &XVECEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0, 1));
      break;

    case 1999:  /* *avx2_gatherdiv8sf_2 */
    case 1998:  /* *avx2_gatherdiv8sf_2 */
    case 1997:  /* *avx2_gatherdiv8si_2 */
    case 1996:  /* *avx2_gatherdiv8si_2 */
    case 1995:  /* *avx2_gatherdiv4sf_2 */
    case 1994:  /* *avx2_gatherdiv4sf_2 */
    case 1993:  /* *avx2_gatherdiv4si_2 */
    case 1992:  /* *avx2_gatherdiv4si_2 */
    case 1991:  /* *avx2_gatherdiv4df_2 */
    case 1990:  /* *avx2_gatherdiv4df_2 */
    case 1989:  /* *avx2_gatherdiv4di_2 */
    case 1988:  /* *avx2_gatherdiv4di_2 */
    case 1987:  /* *avx2_gatherdiv2df_2 */
    case 1986:  /* *avx2_gatherdiv2df_2 */
    case 1985:  /* *avx2_gatherdiv2di_2 */
    case 1984:  /* *avx2_gatherdiv2di_2 */
    case 1967:  /* *avx2_gathersiv8sf_2 */
    case 1966:  /* *avx2_gathersiv8sf_2 */
    case 1965:  /* *avx2_gathersiv8si_2 */
    case 1964:  /* *avx2_gathersiv8si_2 */
    case 1963:  /* *avx2_gathersiv4sf_2 */
    case 1962:  /* *avx2_gathersiv4sf_2 */
    case 1961:  /* *avx2_gathersiv4si_2 */
    case 1960:  /* *avx2_gathersiv4si_2 */
    case 1959:  /* *avx2_gathersiv4df_2 */
    case 1958:  /* *avx2_gathersiv4df_2 */
    case 1957:  /* *avx2_gathersiv4di_2 */
    case 1956:  /* *avx2_gathersiv4di_2 */
    case 1955:  /* *avx2_gathersiv2df_2 */
    case 1954:  /* *avx2_gathersiv2df_2 */
    case 1953:  /* *avx2_gathersiv2di_2 */
    case 1952:  /* *avx2_gathersiv2di_2 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 1), 0), 0, 0));
      ro[3] = *(ro_loc[3] = &XVECEXP (XEXP (XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 1), 0), 0, 1));
      ro[4] = *(ro_loc[4] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 3));
      ro[5] = *(ro_loc[5] = &XVECEXP (XEXP (XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 1), 0), 0, 2));
      ro[6] = *(ro_loc[6] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 1));
      break;

    case 1983:  /* *avx2_gatherdiv8sf */
    case 1982:  /* *avx2_gatherdiv8sf */
    case 1981:  /* *avx2_gatherdiv8si */
    case 1980:  /* *avx2_gatherdiv8si */
    case 1979:  /* *avx2_gatherdiv4sf */
    case 1978:  /* *avx2_gatherdiv4sf */
    case 1977:  /* *avx2_gatherdiv4si */
    case 1976:  /* *avx2_gatherdiv4si */
    case 1975:  /* *avx2_gatherdiv4df */
    case 1974:  /* *avx2_gatherdiv4df */
    case 1973:  /* *avx2_gatherdiv4di */
    case 1972:  /* *avx2_gatherdiv4di */
    case 1971:  /* *avx2_gatherdiv2df */
    case 1970:  /* *avx2_gatherdiv2df */
    case 1969:  /* *avx2_gatherdiv2di */
    case 1968:  /* *avx2_gatherdiv2di */
    case 1951:  /* *avx2_gathersiv8sf */
    case 1950:  /* *avx2_gathersiv8sf */
    case 1949:  /* *avx2_gathersiv8si */
    case 1948:  /* *avx2_gathersiv8si */
    case 1947:  /* *avx2_gathersiv4sf */
    case 1946:  /* *avx2_gathersiv4sf */
    case 1945:  /* *avx2_gathersiv4si */
    case 1944:  /* *avx2_gathersiv4si */
    case 1943:  /* *avx2_gathersiv4df */
    case 1942:  /* *avx2_gathersiv4df */
    case 1941:  /* *avx2_gathersiv4di */
    case 1940:  /* *avx2_gathersiv4di */
    case 1939:  /* *avx2_gathersiv2df */
    case 1938:  /* *avx2_gathersiv2df */
    case 1937:  /* *avx2_gathersiv2di */
    case 1936:  /* *avx2_gathersiv2di */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 0));
      ro[3] = *(ro_loc[3] = &XVECEXP (XEXP (XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 1), 0), 0, 0));
      ro[4] = *(ro_loc[4] = &XVECEXP (XEXP (XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 1), 0), 0, 1));
      ro[5] = *(ro_loc[5] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 3));
      ro[6] = *(ro_loc[6] = &XVECEXP (XEXP (XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 1), 0), 0, 2));
      ro[7] = *(ro_loc[7] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 1));
      break;

    case 1933:  /* *vcvtps2ph */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XVECEXP (XEXP (XEXP (pat, 1), 0), 0, 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (XEXP (pat, 1), 0), 0, 1));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (pat, 1), 1));
      break;

    case 1910:  /* avx2_maskstoreq256 */
    case 1909:  /* avx2_maskstored256 */
    case 1908:  /* avx2_maskstoreq */
    case 1907:  /* avx2_maskstored */
    case 1906:  /* avx_maskstorepd256 */
    case 1905:  /* avx_maskstoreps256 */
    case 1904:  /* avx_maskstorepd */
    case 1903:  /* avx_maskstoreps */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XVECEXP (XEXP (pat, 1), 0, 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (pat, 1), 0, 1));
      recog_data.dup_loc[0] = &XVECEXP (XEXP (pat, 1), 0, 2);
      recog_data.dup_num[0] = 0;
      break;

    case 1902:  /* avx2_maskloadq256 */
    case 1901:  /* avx2_maskloadd256 */
    case 1900:  /* avx2_maskloadq */
    case 1899:  /* avx2_maskloadd */
    case 1898:  /* avx_maskloadpd256 */
    case 1897:  /* avx_maskloadps256 */
    case 1896:  /* avx_maskloadpd */
    case 1895:  /* avx_maskloadps */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XVECEXP (XEXP (pat, 1), 0, 1));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (pat, 1), 0, 0));
      break;

    case 1880:  /* *avx_vperm2f128v4df_nozero */
    case 1879:  /* *avx_vperm2f128v8sf_nozero */
    case 1878:  /* *avx_vperm2f128v8si_nozero */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (pat, 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (pat, 1), 0), 1));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (pat, 1), 1));
      ro[4] = *(ro_loc[4] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 0));
      break;

    case 1870:  /* *avx_vpermilpv2df */
    case 1869:  /* *avx_vpermilpv4df */
    case 1868:  /* *avx_vpermilpv4sf */
    case 1867:  /* *avx_vpermilpv8sf */
    case 1866:  /* *avx_vperm_broadcast_v4df */
    case 1865:  /* *avx_vperm_broadcast_v8sf */
    case 1864:  /* *avx_vperm_broadcast_v4sf */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (pat, 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (pat, 1), 1));
      ro[3] = *(ro_loc[3] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 0));
      break;

    case 1863:  /* avx_vbroadcastf128_v4df */
    case 1862:  /* avx_vbroadcastf128_v8sf */
    case 1861:  /* avx_vbroadcastf128_v4di */
    case 1860:  /* avx_vbroadcastf128_v8si */
    case 1859:  /* avx_vbroadcastf128_v16hi */
    case 1858:  /* avx_vbroadcastf128_v32qi */
    case 1857:  /* avx2_vbroadcasti128_v4di */
    case 1856:  /* avx2_vbroadcasti128_v8si */
    case 1855:  /* avx2_vbroadcasti128_v16hi */
    case 1854:  /* avx2_vbroadcasti128_v32qi */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (pat, 1), 0));
      recog_data.dup_loc[0] = &XEXP (XEXP (pat, 1), 1);
      recog_data.dup_num[0] = 1;
      break;

    case 1818:  /* xop_maskcmp_uns2v2di3 */
    case 1817:  /* xop_maskcmp_uns2v4si3 */
    case 1816:  /* xop_maskcmp_uns2v8hi3 */
    case 1815:  /* xop_maskcmp_uns2v16qi3 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XVECEXP (XEXP (pat, 1), 0, 0));
      ro[2] = *(ro_loc[2] = &XEXP (XVECEXP (XEXP (pat, 1), 0, 0), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XVECEXP (XEXP (pat, 1), 0, 0), 1));
      break;

    case 1814:  /* xop_maskcmp_unsv2di3 */
    case 1813:  /* xop_maskcmp_unsv4si3 */
    case 1812:  /* xop_maskcmp_unsv8hi3 */
    case 1811:  /* xop_maskcmp_unsv16qi3 */
    case 1810:  /* xop_maskcmpv2di3 */
    case 1809:  /* xop_maskcmpv4si3 */
    case 1808:  /* xop_maskcmpv8hi3 */
    case 1807:  /* xop_maskcmpv16qi3 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (pat, 1));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (pat, 1), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (pat, 1), 1));
      break;

    case 1798:  /* xop_shlv2di3 */
    case 1797:  /* xop_shlv4si3 */
    case 1796:  /* xop_shlv8hi3 */
    case 1795:  /* xop_shlv16qi3 */
    case 1794:  /* xop_shav2di3 */
    case 1793:  /* xop_shav4si3 */
    case 1792:  /* xop_shav8hi3 */
    case 1791:  /* xop_shav16qi3 */
    case 1790:  /* xop_vrotlv2di3 */
    case 1789:  /* xop_vrotlv4si3 */
    case 1788:  /* xop_vrotlv8hi3 */
    case 1787:  /* xop_vrotlv16qi3 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (pat, 1), 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (pat, 1), 0), 0));
      recog_data.dup_loc[0] = &XEXP (XEXP (XEXP (pat, 1), 1), 1);
      recog_data.dup_num[0] = 2;
      recog_data.dup_loc[1] = &XEXP (XEXP (XEXP (pat, 1), 2), 0);
      recog_data.dup_num[1] = 1;
      recog_data.dup_loc[2] = &XEXP (XEXP (XEXP (XEXP (pat, 1), 2), 1), 0);
      recog_data.dup_num[2] = 2;
      break;

    case 1778:  /* xop_pperm_pack_v8hi_v16qi */
    case 1777:  /* xop_pperm_pack_v4si_v8hi */
    case 1776:  /* xop_pperm_pack_v2di_v4si */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XVECEXP (pat, 0, 1), 0));
      break;

    case 1768:  /* xop_phaddubq */
    case 1762:  /* xop_phaddbq */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 0), 0), 0));
      recog_data.dup_loc[0] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 1), 0), 0);
      recog_data.dup_num[0] = 1;
      recog_data.dup_loc[1] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 1), 0), 0), 0);
      recog_data.dup_num[1] = 1;
      recog_data.dup_loc[2] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 1), 1), 0), 0);
      recog_data.dup_num[2] = 1;
      recog_data.dup_loc[3] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 0), 0), 0), 0);
      recog_data.dup_num[3] = 1;
      recog_data.dup_loc[4] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 0), 1), 0), 0);
      recog_data.dup_num[4] = 1;
      recog_data.dup_loc[5] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 1), 0), 0), 0);
      recog_data.dup_num[5] = 1;
      recog_data.dup_loc[6] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 1), 1), 0), 0);
      recog_data.dup_num[6] = 1;
      break;

    case 1770:  /* xop_phadduwq */
    case 1767:  /* xop_phaddubd */
    case 1764:  /* xop_phaddwq */
    case 1761:  /* xop_phaddbd */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 0), 0));
      recog_data.dup_loc[0] = &XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 1), 0), 0);
      recog_data.dup_num[0] = 1;
      recog_data.dup_loc[1] = &XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 0), 0), 0);
      recog_data.dup_num[1] = 1;
      recog_data.dup_loc[2] = &XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 1), 0), 0);
      recog_data.dup_num[2] = 1;
      break;

    case 1774:  /* xop_phsubdq */
    case 1773:  /* xop_phsubwd */
    case 1772:  /* xop_phsubbw */
    case 1771:  /* xop_phaddudq */
    case 1769:  /* xop_phadduwd */
    case 1766:  /* xop_phaddubw */
    case 1765:  /* xop_phadddq */
    case 1763:  /* xop_phaddwd */
    case 1760:  /* xop_phaddbw */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 0));
      recog_data.dup_loc[0] = &XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 0), 0);
      recog_data.dup_num[0] = 1;
      break;

    case 1759:  /* xop_pcmov_v2df */
    case 1758:  /* xop_pcmov_v4df256 */
    case 1757:  /* xop_pcmov_v4sf */
    case 1756:  /* xop_pcmov_v8sf256 */
    case 1755:  /* xop_pcmov_v2di */
    case 1754:  /* xop_pcmov_v4di256 */
    case 1753:  /* xop_pcmov_v4si */
    case 1752:  /* xop_pcmov_v8si256 */
    case 1751:  /* xop_pcmov_v8hi */
    case 1750:  /* xop_pcmov_v16hi256 */
    case 1749:  /* xop_pcmov_v16qi */
    case 1748:  /* xop_pcmov_v32qi256 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (pat, 1), 1));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (pat, 1), 2));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (pat, 1), 0));
      break;

    case 1747:  /* xop_pmadcswd */
    case 1746:  /* xop_pmadcsswd */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 0), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 1), 0), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (pat, 1), 1));
      recog_data.dup_loc[0] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 1), 0), 0), 0);
      recog_data.dup_num[0] = 1;
      recog_data.dup_loc[1] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 1), 1), 0), 0);
      recog_data.dup_num[1] = 2;
      break;

    case 1745:  /* xop_pmacswd */
    case 1744:  /* xop_pmacsswd */
    case 1743:  /* xop_pmacsdqh */
    case 1742:  /* xop_pmacsdql */
    case 1741:  /* xop_pmacssdqh */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 1), 0), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (pat, 1), 1));
      break;

    case 1740:  /* xop_pmacssdql */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 1), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (pat, 1), 1));
      break;

    case 1735:  /* sse4_2_pcmpistr_cconly */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 2), 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 0));
      ro[3] = *(ro_loc[3] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 1));
      ro[4] = *(ro_loc[4] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 2));
      break;

    case 1734:  /* sse4_2_pcmpistrm */
    case 1733:  /* sse4_2_pcmpistri */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 1));
      ro[3] = *(ro_loc[3] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 2));
      recog_data.dup_loc[0] = &XVECEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0, 2);
      recog_data.dup_num[0] = 3;
      recog_data.dup_loc[1] = &XVECEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0, 1);
      recog_data.dup_num[1] = 2;
      recog_data.dup_loc[2] = &XVECEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0, 0);
      recog_data.dup_num[2] = 1;
      break;

    case 1732:  /* sse4_2_pcmpistr */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 0));
      ro[3] = *(ro_loc[3] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 1));
      ro[4] = *(ro_loc[4] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 2));
      recog_data.dup_loc[0] = &XVECEXP (XEXP (XVECEXP (pat, 0, 2), 1), 0, 2);
      recog_data.dup_num[0] = 4;
      recog_data.dup_loc[1] = &XVECEXP (XEXP (XVECEXP (pat, 0, 2), 1), 0, 1);
      recog_data.dup_num[1] = 3;
      recog_data.dup_loc[2] = &XVECEXP (XEXP (XVECEXP (pat, 0, 2), 1), 0, 0);
      recog_data.dup_num[2] = 2;
      recog_data.dup_loc[3] = &XVECEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0, 2);
      recog_data.dup_num[3] = 4;
      recog_data.dup_loc[4] = &XVECEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0, 1);
      recog_data.dup_num[4] = 3;
      recog_data.dup_loc[5] = &XVECEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0, 0);
      recog_data.dup_num[5] = 2;
      break;

    case 1731:  /* sse4_2_pcmpestr_cconly */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 2), 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 0));
      ro[3] = *(ro_loc[3] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 1));
      ro[4] = *(ro_loc[4] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 2));
      ro[5] = *(ro_loc[5] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 3));
      ro[6] = *(ro_loc[6] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 4));
      break;

    case 1730:  /* sse4_2_pcmpestrm */
    case 1729:  /* sse4_2_pcmpestri */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 1));
      ro[3] = *(ro_loc[3] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 2));
      ro[4] = *(ro_loc[4] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 3));
      ro[5] = *(ro_loc[5] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 4));
      recog_data.dup_loc[0] = &XVECEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0, 4);
      recog_data.dup_num[0] = 5;
      recog_data.dup_loc[1] = &XVECEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0, 3);
      recog_data.dup_num[1] = 4;
      recog_data.dup_loc[2] = &XVECEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0, 2);
      recog_data.dup_num[2] = 3;
      recog_data.dup_loc[3] = &XVECEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0, 1);
      recog_data.dup_num[3] = 2;
      recog_data.dup_loc[4] = &XVECEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0, 0);
      recog_data.dup_num[4] = 1;
      break;

    case 1728:  /* sse4_2_pcmpestr */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 0));
      ro[3] = *(ro_loc[3] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 1));
      ro[4] = *(ro_loc[4] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 2));
      ro[5] = *(ro_loc[5] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 3));
      ro[6] = *(ro_loc[6] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 4));
      recog_data.dup_loc[0] = &XVECEXP (XEXP (XVECEXP (pat, 0, 2), 1), 0, 4);
      recog_data.dup_num[0] = 6;
      recog_data.dup_loc[1] = &XVECEXP (XEXP (XVECEXP (pat, 0, 2), 1), 0, 3);
      recog_data.dup_num[1] = 5;
      recog_data.dup_loc[2] = &XVECEXP (XEXP (XVECEXP (pat, 0, 2), 1), 0, 2);
      recog_data.dup_num[2] = 4;
      recog_data.dup_loc[3] = &XVECEXP (XEXP (XVECEXP (pat, 0, 2), 1), 0, 1);
      recog_data.dup_num[3] = 3;
      recog_data.dup_loc[4] = &XVECEXP (XEXP (XVECEXP (pat, 0, 2), 1), 0, 0);
      recog_data.dup_num[4] = 2;
      recog_data.dup_loc[5] = &XVECEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0, 4);
      recog_data.dup_num[5] = 6;
      recog_data.dup_loc[6] = &XVECEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0, 3);
      recog_data.dup_num[6] = 5;
      recog_data.dup_loc[7] = &XVECEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0, 2);
      recog_data.dup_num[7] = 4;
      recog_data.dup_loc[8] = &XVECEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0, 1);
      recog_data.dup_num[8] = 3;
      recog_data.dup_loc[9] = &XVECEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0, 0);
      recog_data.dup_num[9] = 2;
      break;

    case 1727:  /* sse4_1_roundsd */
    case 1726:  /* sse4_1_roundss */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (pat, 1), 1));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (XEXP (pat, 1), 0), 0, 0));
      ro[3] = *(ro_loc[3] = &XVECEXP (XEXP (XEXP (pat, 1), 0), 0, 1));
      break;

    case 1721:  /* sse4_1_ptest */
    case 1720:  /* avx_ptest256 */
    case 1719:  /* avx_vtestpd */
    case 1718:  /* avx_vtestpd256 */
    case 1717:  /* avx_vtestps */
    case 1716:  /* avx_vtestps256 */
      ro[0] = *(ro_loc[0] = &XVECEXP (XEXP (pat, 1), 0, 0));
      ro[1] = *(ro_loc[1] = &XVECEXP (XEXP (pat, 1), 0, 1));
      break;

    case 1690:  /* avx2_pblenddv4si */
    case 1689:  /* avx2_pblenddv8si */
    case 1688:  /* *avx2_pblendw */
    case 1687:  /* sse4_1_pblendw */
    case 1670:  /* sse4_1_blendpd */
    case 1669:  /* avx_blendpd256 */
    case 1668:  /* sse4_1_blendps */
    case 1667:  /* avx_blendps256 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (pat, 1), 1));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (pat, 1), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (pat, 1), 2));
      break;

    case 1826:  /* xop_vpermil2v2df3 */
    case 1825:  /* xop_vpermil2v4df3 */
    case 1824:  /* xop_vpermil2v4sf3 */
    case 1823:  /* xop_vpermil2v8sf3 */
    case 1665:  /* sse4a_insertqi */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XVECEXP (XEXP (pat, 1), 0, 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (pat, 1), 0, 1));
      ro[3] = *(ro_loc[3] = &XVECEXP (XEXP (pat, 1), 0, 2));
      ro[4] = *(ro_loc[4] = &XVECEXP (XEXP (pat, 1), 0, 3));
      break;

    case 1634:  /* *ssse3_pmulhrswv4hi3 */
    case 1633:  /* *ssse3_pmulhrswv8hi3 */
    case 1632:  /* *avx2_umulhrswv16hi3 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 0), 0), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 0), 0), 1), 0));
      break;

    case 1627:  /* ssse3_phsubswv8hi3 */
    case 1623:  /* avx2_phsubdv8si3 */
    case 1621:  /* ssse3_phsubwv8hi3 */
    case 1618:  /* ssse3_phaddswv8hi3 */
    case 1614:  /* avx2_phadddv8si3 */
    case 1612:  /* ssse3_phaddwv8hi3 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 0), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 0), 0), 0), 0));
      recog_data.dup_loc[0] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 0), 1), 0);
      recog_data.dup_num[0] = 1;
      recog_data.dup_loc[1] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 1), 0), 0);
      recog_data.dup_num[1] = 1;
      recog_data.dup_loc[2] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 1), 1), 0);
      recog_data.dup_num[2] = 1;
      recog_data.dup_loc[3] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 1), 0), 0), 0);
      recog_data.dup_num[3] = 1;
      recog_data.dup_loc[4] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 1), 0), 1), 0);
      recog_data.dup_num[4] = 1;
      recog_data.dup_loc[5] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 1), 1), 0), 0);
      recog_data.dup_num[5] = 1;
      recog_data.dup_loc[6] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 1), 1), 1), 0);
      recog_data.dup_num[6] = 1;
      recog_data.dup_loc[7] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 0), 0), 1), 0);
      recog_data.dup_num[7] = 2;
      recog_data.dup_loc[8] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 0), 1), 0), 0);
      recog_data.dup_num[8] = 2;
      recog_data.dup_loc[9] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 0), 1), 1), 0);
      recog_data.dup_num[9] = 2;
      recog_data.dup_loc[10] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 1), 0), 0), 0);
      recog_data.dup_num[10] = 2;
      recog_data.dup_loc[11] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 1), 0), 1), 0);
      recog_data.dup_num[11] = 2;
      recog_data.dup_loc[12] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 1), 1), 0), 0);
      recog_data.dup_num[12] = 2;
      recog_data.dup_loc[13] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 1), 1), 1), 0);
      recog_data.dup_num[13] = 2;
      break;

    case 1626:  /* avx2_phsubswv16hi3 */
    case 1620:  /* avx2_phsubwv16hi3 */
    case 1617:  /* avx2_phaddswv16hi3 */
    case 1611:  /* avx2_phaddwv16hi3 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 0), 0), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 0), 0), 0), 0), 0));
      recog_data.dup_loc[0] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 0), 0), 1), 0);
      recog_data.dup_num[0] = 1;
      recog_data.dup_loc[1] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 0), 1), 0), 0);
      recog_data.dup_num[1] = 1;
      recog_data.dup_loc[2] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 0), 1), 1), 0);
      recog_data.dup_num[2] = 1;
      recog_data.dup_loc[3] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 1), 0), 0), 0);
      recog_data.dup_num[3] = 1;
      recog_data.dup_loc[4] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 1), 0), 1), 0);
      recog_data.dup_num[4] = 1;
      recog_data.dup_loc[5] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 1), 1), 0), 0);
      recog_data.dup_num[5] = 1;
      recog_data.dup_loc[6] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 1), 1), 1), 0);
      recog_data.dup_num[6] = 1;
      recog_data.dup_loc[7] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 1), 0), 0), 0), 0);
      recog_data.dup_num[7] = 1;
      recog_data.dup_loc[8] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 1), 0), 0), 1), 0);
      recog_data.dup_num[8] = 1;
      recog_data.dup_loc[9] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 1), 0), 1), 0), 0);
      recog_data.dup_num[9] = 1;
      recog_data.dup_loc[10] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 1), 0), 1), 1), 0);
      recog_data.dup_num[10] = 1;
      recog_data.dup_loc[11] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 1), 1), 0), 0), 0);
      recog_data.dup_num[11] = 1;
      recog_data.dup_loc[12] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 1), 1), 0), 1), 0);
      recog_data.dup_num[12] = 1;
      recog_data.dup_loc[13] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 1), 1), 1), 0), 0);
      recog_data.dup_num[13] = 1;
      recog_data.dup_loc[14] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 1), 1), 1), 1), 0);
      recog_data.dup_num[14] = 1;
      recog_data.dup_loc[15] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 0), 0), 0), 1), 0);
      recog_data.dup_num[15] = 2;
      recog_data.dup_loc[16] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 0), 0), 1), 0), 0);
      recog_data.dup_num[16] = 2;
      recog_data.dup_loc[17] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 0), 0), 1), 1), 0);
      recog_data.dup_num[17] = 2;
      recog_data.dup_loc[18] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 0), 1), 0), 0), 0);
      recog_data.dup_num[18] = 2;
      recog_data.dup_loc[19] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 0), 1), 0), 1), 0);
      recog_data.dup_num[19] = 2;
      recog_data.dup_loc[20] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 0), 1), 1), 0), 0);
      recog_data.dup_num[20] = 2;
      recog_data.dup_loc[21] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 0), 1), 1), 1), 0);
      recog_data.dup_num[21] = 2;
      recog_data.dup_loc[22] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 1), 0), 0), 0), 0);
      recog_data.dup_num[22] = 2;
      recog_data.dup_loc[23] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 1), 0), 0), 1), 0);
      recog_data.dup_num[23] = 2;
      recog_data.dup_loc[24] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 1), 0), 1), 0), 0);
      recog_data.dup_num[24] = 2;
      recog_data.dup_loc[25] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 1), 0), 1), 1), 0);
      recog_data.dup_num[25] = 2;
      recog_data.dup_loc[26] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 1), 1), 0), 0), 0);
      recog_data.dup_num[26] = 2;
      recog_data.dup_loc[27] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 1), 1), 0), 1), 0);
      recog_data.dup_num[27] = 2;
      recog_data.dup_loc[28] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 1), 1), 1), 0), 0);
      recog_data.dup_num[28] = 2;
      recog_data.dup_loc[29] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 1), 1), 1), 1), 0);
      recog_data.dup_num[29] = 2;
      break;

    case 1608:  /* sse3_mwait */
      ro[0] = *(ro_loc[0] = &XVECEXP (pat, 0, 0));
      ro[1] = *(ro_loc[1] = &XVECEXP (pat, 0, 1));
      break;

    case 1575:  /* sse2_pshufhw_1 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (pat, 1), 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 4));
      ro[3] = *(ro_loc[3] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 5));
      ro[4] = *(ro_loc[4] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 6));
      ro[5] = *(ro_loc[5] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 7));
      break;

    case 1574:  /* avx2_pshufhw_1 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (pat, 1), 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 4));
      ro[3] = *(ro_loc[3] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 5));
      ro[4] = *(ro_loc[4] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 6));
      ro[5] = *(ro_loc[5] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 7));
      ro[6] = *(ro_loc[6] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 12));
      ro[7] = *(ro_loc[7] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 13));
      ro[8] = *(ro_loc[8] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 14));
      ro[9] = *(ro_loc[9] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 15));
      break;

    case 1572:  /* avx2_pshuflw_1 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (pat, 1), 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 0));
      ro[3] = *(ro_loc[3] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 1));
      ro[4] = *(ro_loc[4] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 2));
      ro[5] = *(ro_loc[5] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 3));
      ro[6] = *(ro_loc[6] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 8));
      ro[7] = *(ro_loc[7] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 9));
      ro[8] = *(ro_loc[8] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 10));
      ro[9] = *(ro_loc[9] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 11));
      break;

    case 1570:  /* avx2_pshufd_1 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (pat, 1), 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 0));
      ro[3] = *(ro_loc[3] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 1));
      ro[4] = *(ro_loc[4] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 2));
      ro[5] = *(ro_loc[5] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 3));
      ro[6] = *(ro_loc[6] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 4));
      ro[7] = *(ro_loc[7] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 5));
      ro[8] = *(ro_loc[8] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 6));
      ro[9] = *(ro_loc[9] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 7));
      break;

    case 1382:  /* sse2_shufpd_v2df */
    case 1381:  /* sse2_shufpd_v2di */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (pat, 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (pat, 1), 0), 1));
      ro[3] = *(ro_loc[3] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 0));
      ro[4] = *(ro_loc[4] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 1));
      break;

    case 1578:  /* *vec_ext_v4si_mem */
    case 1569:  /* *sse4_1_pextrq */
    case 1567:  /* *sse4_1_pextrd */
    case 1566:  /* *sse4_1_pextrw_memory */
    case 1563:  /* *sse4_1_pextrb_memory */
    case 1359:  /* *vec_extract_v4sf_mem */
    case 1358:  /* *sse4_1_extractps */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (pat, 1), 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 0));
      break;

    case 1356:  /* sse4_1_insertps */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XVECEXP (XEXP (pat, 1), 0, 1));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (pat, 1), 0, 0));
      ro[3] = *(ro_loc[3] = &XVECEXP (XEXP (pat, 1), 0, 2));
      break;

    case 1576:  /* sse2_loadld */
    case 1354:  /* vec_setv4sf_0 */
    case 1353:  /* vec_setv4si_0 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (pat, 1), 1));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (pat, 1), 0), 0));
      break;

    case 1389:  /* sse2_movsd */
    case 1346:  /* sse_movss */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (pat, 1), 1));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (pat, 1), 0));
      break;

    case 1893:  /* vec_set_lo_v32qi */
    case 1891:  /* vec_set_lo_v16hi */
    case 1888:  /* vec_set_lo_v8sf */
    case 1887:  /* vec_set_lo_v8si */
    case 1884:  /* vec_set_lo_v4df */
    case 1883:  /* vec_set_lo_v4di */
    case 1881:  /* avx2_vec_set_lo_v4di */
    case 1388:  /* sse2_loadlpd */
    case 1345:  /* sse_loadlps */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (pat, 1), 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (pat, 1), 0));
      break;

    case 1376:  /* avx_shufpd256_1 */
    case 1341:  /* sse_shufps_v4sf */
    case 1340:  /* sse_shufps_v4si */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (pat, 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (pat, 1), 0), 1));
      ro[3] = *(ro_loc[3] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 0));
      ro[4] = *(ro_loc[4] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 1));
      ro[5] = *(ro_loc[5] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 2));
      ro[6] = *(ro_loc[6] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 3));
      break;

    case 1339:  /* avx_shufps256_1 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (pat, 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (pat, 1), 0), 1));
      ro[3] = *(ro_loc[3] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 0));
      ro[4] = *(ro_loc[4] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 1));
      ro[5] = *(ro_loc[5] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 2));
      ro[6] = *(ro_loc[6] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 3));
      ro[7] = *(ro_loc[7] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 4));
      ro[8] = *(ro_loc[8] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 5));
      ro[9] = *(ro_loc[9] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 6));
      ro[10] = *(ro_loc[10] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 7));
      break;

    case 1338:  /* sse3_movsldup */
    case 1337:  /* avx_movsldup256 */
    case 1336:  /* sse3_movshdup */
    case 1335:  /* avx_movshdup256 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (pat, 1), 0), 0));
      recog_data.dup_loc[0] = &XEXP (XEXP (XEXP (pat, 1), 0), 1);
      recog_data.dup_num[0] = 1;
      break;

    case 1930:  /* vcvtph2ps */
    case 1286:  /* sse_cvtps2pi */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XVECEXP (XEXP (XEXP (pat, 1), 0), 0, 0));
      break;

    case 1323:  /* sse2_cvtss2sd */
    case 1322:  /* sse2_cvtsd2ss */
    case 1306:  /* sse2_cvtsi2sdq */
    case 1305:  /* sse2_cvtsi2sd */
    case 1289:  /* sse_cvtsi2ssq */
    case 1288:  /* sse_cvtsi2ss */
    case 1285:  /* sse_cvtpi2ps */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (pat, 1), 1));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 0));
      break;

    case 1284:  /* *fma4i_vmfnmsub_v2df */
    case 1283:  /* *fma4i_vmfnmsub_v4sf */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (pat, 1), 0), 1));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 2), 0));
      ro[4] = *(ro_loc[4] = &XEXP (XEXP (pat, 1), 1));
      break;

    case 1282:  /* *fma4i_vmfnmadd_v2df */
    case 1281:  /* *fma4i_vmfnmadd_v4sf */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (pat, 1), 0), 1));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XEXP (pat, 1), 0), 2));
      ro[4] = *(ro_loc[4] = &XEXP (XEXP (pat, 1), 1));
      break;

    case 1280:  /* *fma4i_vmfmsub_v2df */
    case 1279:  /* *fma4i_vmfmsub_v4sf */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (pat, 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (pat, 1), 0), 1));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 2), 0));
      ro[4] = *(ro_loc[4] = &XEXP (XEXP (pat, 1), 1));
      break;

    case 1278:  /* *fma4i_vmfmadd_v2df */
    case 1277:  /* *fma4i_vmfmadd_v4sf */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (pat, 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (pat, 1), 0), 1));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XEXP (pat, 1), 0), 2));
      ro[4] = *(ro_loc[4] = &XEXP (XEXP (pat, 1), 1));
      break;

    case 1276:  /* *fmai_fnmsub_v2df */
    case 1275:  /* *fmai_fnmsub_v4sf */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (pat, 1), 0), 1));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 2), 0));
      recog_data.dup_loc[0] = &XEXP (XEXP (pat, 1), 1);
      recog_data.dup_num[0] = 1;
      break;

    case 1274:  /* *fmai_fnmadd_v2df */
    case 1273:  /* *fmai_fnmadd_v4sf */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (pat, 1), 0), 1));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XEXP (pat, 1), 0), 2));
      recog_data.dup_loc[0] = &XEXP (XEXP (pat, 1), 1);
      recog_data.dup_num[0] = 1;
      break;

    case 1272:  /* *fmai_fmsub_v2df */
    case 1271:  /* *fmai_fmsub_v4sf */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (pat, 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (pat, 1), 0), 1));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 2), 0));
      recog_data.dup_loc[0] = &XEXP (XEXP (pat, 1), 1);
      recog_data.dup_num[0] = 1;
      break;

    case 1270:  /* *fmai_fmadd_v2df */
    case 1269:  /* *fmai_fmadd_v4sf */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (pat, 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (pat, 1), 0), 1));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XEXP (pat, 1), 0), 2));
      recog_data.dup_loc[0] = &XEXP (XEXP (pat, 1), 1);
      recog_data.dup_num[0] = 1;
      break;

    case 1268:  /* *fma_fmsubadd_v2df */
    case 1267:  /* *fma_fmsubadd_v4df */
    case 1266:  /* *fma_fmsubadd_v4sf */
    case 1265:  /* *fma_fmsubadd_v8sf */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XVECEXP (XEXP (pat, 1), 0, 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (pat, 1), 0, 1));
      ro[3] = *(ro_loc[3] = &XEXP (XVECEXP (XEXP (pat, 1), 0, 2), 0));
      break;

    case 1260:  /* *fma_fnmsub_v4df */
    case 1259:  /* *fma_fnmsub_v8sf */
    case 1258:  /* *fma_fnmsub_v2df */
    case 1257:  /* *fma_fnmsub_v4sf */
    case 1256:  /* *fma_fnmsub_df */
    case 1255:  /* *fma_fnmsub_sf */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (pat, 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (pat, 1), 1));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XEXP (pat, 1), 2), 0));
      break;

    case 1254:  /* *fma_fnmadd_v4df */
    case 1253:  /* *fma_fnmadd_v8sf */
    case 1252:  /* *fma_fnmadd_v2df */
    case 1251:  /* *fma_fnmadd_v4sf */
    case 1250:  /* *fma_fnmadd_df */
    case 1249:  /* *fma_fnmadd_sf */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (pat, 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (pat, 1), 1));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (pat, 1), 2));
      break;

    case 1248:  /* *fma_fmsub_v4df */
    case 1247:  /* *fma_fmsub_v8sf */
    case 1246:  /* *fma_fmsub_v2df */
    case 1245:  /* *fma_fmsub_v4sf */
    case 1244:  /* *fma_fmsub_df */
    case 1243:  /* *fma_fmsub_sf */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (pat, 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (pat, 1), 1));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XEXP (pat, 1), 2), 0));
      break;

    case 1212:  /* sse2_ucomi */
    case 1211:  /* sse_ucomi */
    case 1210:  /* sse2_comi */
    case 1209:  /* sse_comi */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (XEXP (pat, 1), 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (pat, 1), 1), 0));
      break;

    case 1208:  /* sse2_vmmaskcmpv2df3 */
    case 1207:  /* sse_vmmaskcmpv4sf3 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (pat, 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (pat, 1), 0), 1));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (pat, 1), 0));
      recog_data.dup_loc[0] = &XEXP (XEXP (pat, 1), 1);
      recog_data.dup_num[0] = 1;
      break;

    case 1198:  /* avx_vmcmpv2df3 */
    case 1197:  /* avx_vmcmpv4sf3 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XVECEXP (XEXP (XEXP (pat, 1), 0), 0, 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (XEXP (pat, 1), 0), 0, 1));
      ro[3] = *(ro_loc[3] = &XVECEXP (XEXP (XEXP (pat, 1), 0), 0, 2));
      recog_data.dup_loc[0] = &XEXP (XEXP (pat, 1), 1);
      recog_data.dup_num[0] = 1;
      break;

    case 1628:  /* ssse3_phsubswv4hi3 */
    case 1624:  /* ssse3_phsubdv4si3 */
    case 1622:  /* ssse3_phsubwv4hi3 */
    case 1619:  /* ssse3_phaddswv4hi3 */
    case 1615:  /* ssse3_phadddv4si3 */
    case 1613:  /* ssse3_phaddwv4hi3 */
    case 1192:  /* sse3_hsubv4sf3 */
    case 1191:  /* sse3_haddv4sf3 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 0), 0), 0));
      recog_data.dup_loc[0] = &XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 1), 0);
      recog_data.dup_num[0] = 1;
      recog_data.dup_loc[1] = &XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 1), 0), 0);
      recog_data.dup_num[1] = 1;
      recog_data.dup_loc[2] = &XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 1), 1), 0);
      recog_data.dup_num[2] = 1;
      recog_data.dup_loc[3] = &XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 0), 1), 0);
      recog_data.dup_num[3] = 2;
      recog_data.dup_loc[4] = &XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 1), 0), 0);
      recog_data.dup_num[4] = 2;
      recog_data.dup_loc[5] = &XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 1), 1), 0);
      recog_data.dup_num[5] = 2;
      break;

    case 1190:  /* avx_hsubv8sf3 */
    case 1189:  /* avx_haddv8sf3 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 0), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 1), 0), 0), 0));
      recog_data.dup_loc[0] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 0), 1), 0);
      recog_data.dup_num[0] = 1;
      recog_data.dup_loc[1] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 1), 0), 0);
      recog_data.dup_num[1] = 1;
      recog_data.dup_loc[2] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 1), 1), 0);
      recog_data.dup_num[2] = 1;
      recog_data.dup_loc[3] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 1), 0), 1), 0);
      recog_data.dup_num[3] = 2;
      recog_data.dup_loc[4] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 1), 1), 0), 0);
      recog_data.dup_num[4] = 2;
      recog_data.dup_loc[5] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 1), 1), 1), 0);
      recog_data.dup_num[5] = 2;
      recog_data.dup_loc[6] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 0), 0), 0), 0);
      recog_data.dup_num[6] = 1;
      recog_data.dup_loc[7] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 0), 0), 1), 0);
      recog_data.dup_num[7] = 1;
      recog_data.dup_loc[8] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 0), 1), 0), 0);
      recog_data.dup_num[8] = 1;
      recog_data.dup_loc[9] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 0), 1), 1), 0);
      recog_data.dup_num[9] = 1;
      recog_data.dup_loc[10] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 1), 0), 0), 0);
      recog_data.dup_num[10] = 2;
      recog_data.dup_loc[11] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 1), 0), 1), 0);
      recog_data.dup_num[11] = 2;
      recog_data.dup_loc[12] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 1), 1), 0), 0);
      recog_data.dup_num[12] = 2;
      recog_data.dup_loc[13] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 1), 1), 1), 0);
      recog_data.dup_num[13] = 2;
      break;

    case 1186:  /* avx_hsubv4df3 */
    case 1185:  /* avx_haddv4df3 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 1), 0), 0));
      recog_data.dup_loc[0] = &XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 1), 0);
      recog_data.dup_num[0] = 1;
      recog_data.dup_loc[1] = &XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 1), 1), 0);
      recog_data.dup_num[1] = 2;
      recog_data.dup_loc[2] = &XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 0), 0), 0);
      recog_data.dup_num[2] = 1;
      recog_data.dup_loc[3] = &XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 0), 1), 0);
      recog_data.dup_num[3] = 1;
      recog_data.dup_loc[4] = &XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 1), 0), 0);
      recog_data.dup_num[4] = 2;
      recog_data.dup_loc[5] = &XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 1), 1), 0);
      recog_data.dup_num[5] = 2;
      break;

    case 1806:  /* *xop_vmfrczv2df2 */
    case 1805:  /* *xop_vmfrczv4sf2 */
    case 1318:  /* *sse2_cvtpd2dq */
    case 1317:  /* *avx_cvtpd2dq256_2 */
    case 1152:  /* sse_vmrsqrtv4sf2 */
    case 1143:  /* sse_vmrcpv4sf2 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XVECEXP (XEXP (XEXP (pat, 1), 0), 0, 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (pat, 1), 1));
      break;

    case 1172:  /* sse2_vmsminv2df3 */
    case 1171:  /* sse2_vmsmaxv2df3 */
    case 1170:  /* sse_vmsminv4sf3 */
    case 1169:  /* sse_vmsmaxv4sf3 */
    case 1140:  /* sse2_vmdivv2df3 */
    case 1139:  /* sse_vmdivv4sf3 */
    case 1134:  /* sse2_vmmulv2df3 */
    case 1133:  /* sse_vmmulv4sf3 */
    case 1128:  /* sse2_vmsubv2df3 */
    case 1127:  /* sse2_vmaddv2df3 */
    case 1126:  /* sse_vmsubv4sf3 */
    case 1125:  /* sse_vmaddv4sf3 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (pat, 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (pat, 1), 0), 1));
      recog_data.dup_loc[0] = &XEXP (XEXP (pat, 1), 1);
      recog_data.dup_num[0] = 1;
      break;

    case 1834:  /* *avx_vzeroall */
    case 1074:  /* *mmx_femms */
    case 1073:  /* *mmx_emms */
      ro[0] = *(ro_loc[0] = &PATTERN (insn));
      break;

    case 1604:  /* *sse2_maskmovdqu */
    case 1603:  /* *sse2_maskmovdqu */
    case 1072:  /* *mmx_maskmovq */
    case 1071:  /* *mmx_maskmovq */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (pat, 0), 0));
      ro[1] = *(ro_loc[1] = &XVECEXP (XEXP (pat, 1), 0, 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (pat, 1), 0, 1));
      recog_data.dup_loc[0] = &XEXP (XVECEXP (XEXP (pat, 1), 0, 2), 0);
      recog_data.dup_num[0] = 0;
      break;

    case 1847:  /* avx2_permv4di_1 */
    case 1573:  /* sse2_pshuflw_1 */
    case 1571:  /* sse2_pshufd_1 */
    case 1060:  /* mmx_pshufw_1 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (pat, 1), 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 0));
      ro[3] = *(ro_loc[3] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 1));
      ro[4] = *(ro_loc[4] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 2));
      ro[5] = *(ro_loc[5] = &XVECEXP (XEXP (XEXP (pat, 1), 1), 0, 3));
      break;

    case 1568:  /* *sse4_1_pextrd_zext */
    case 1565:  /* *sse2_pextrw_di */
    case 1564:  /* *sse2_pextrw_si */
    case 1562:  /* *sse4_1_pextrb_di */
    case 1561:  /* *sse4_1_pextrb_si */
    case 1059:  /* mmx_pextrw */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (pat, 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (XEXP (XEXP (pat, 1), 0), 1), 0, 0));
      break;

    case 1560:  /* sse4_1_pinsrq */
    case 1559:  /* sse4_1_pinsrd */
    case 1558:  /* sse2_pinsrw */
    case 1557:  /* sse4_1_pinsrb */
    case 1355:  /* *vec_setv4sf_sse4_1 */
    case 1058:  /* *mmx_pinsrw */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (pat, 1), 1));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (pat, 1), 0), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (pat, 1), 2));
      break;

    case 1684:  /* sse4_1_packusdw */
    case 1683:  /* avx2_packusdw */
    case 1544:  /* sse2_packuswb */
    case 1543:  /* avx2_packuswb */
    case 1542:  /* sse2_packssdw */
    case 1541:  /* avx2_packssdw */
    case 1540:  /* sse2_packsswb */
    case 1539:  /* avx2_packsswb */
    case 1051:  /* mmx_packuswb */
    case 1050:  /* mmx_packssdw */
    case 1049:  /* mmx_packsswb */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (pat, 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (pat, 1), 1), 0));
      break;

    case 1435:  /* *sse4_1_mulv2siv2di3 */
    case 1434:  /* *avx2_mulv4siv4di3 */
    case 1433:  /* *sse2_umulv2siv2di3 */
    case 1432:  /* *avx_umulv4siv4di3 */
    case 1018:  /* *sse2_umulv1siv1di3 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 0), 0));
      break;

    case 1594:  /* *sse2_uavgv8hi3 */
    case 1593:  /* *avx2_uavgv16hi3 */
    case 1592:  /* *sse2_uavgv16qi3 */
    case 1591:  /* *avx2_uavgv32qi3 */
    case 1068:  /* *mmx_uavgv4hi3 */
    case 1067:  /* *mmx_uavgv8qi3 */
    case 1017:  /* *mmx_pmulhrwv4hi3 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 0), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 0), 1), 0));
      break;

    case 1631:  /* ssse3_pmaddubsw */
    case 1630:  /* ssse3_pmaddubsw128 */
    case 1629:  /* avx2_pmaddubsw256 */
    case 1437:  /* *sse2_pmaddwd */
    case 1436:  /* *avx2_pmaddwd */
    case 1016:  /* *mmx_pmaddwd */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 1), 0), 0));
      recog_data.dup_loc[0] = &XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 0), 0), 0);
      recog_data.dup_num[0] = 1;
      recog_data.dup_loc[1] = &XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 1), 0), 0);
      recog_data.dup_num[1] = 2;
      break;

    case 1431:  /* *umulv8hi3_highpart */
    case 1430:  /* *smulv8hi3_highpart */
    case 1429:  /* *umulv16hi3_highpart */
    case 1428:  /* *smulv16hi3_highpart */
    case 1015:  /* *mmx_umulv4hi3_highpart */
    case 1014:  /* *mmx_smulv4hi3_highpart */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 1), 0));
      break;

    case 990:  /* mmx_pi2fw */
    case 989:  /* mmx_pf2iw */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 0));
      break;

    case 1184:  /* sse3_addsubv4sf3 */
    case 1183:  /* avx_addsubv8sf3 */
    case 1182:  /* sse3_addsubv2df3 */
    case 1181:  /* avx_addsubv4df3 */
    case 984:  /* mmx_addsubv2sf3 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (pat, 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (pat, 1), 0), 1));
      recog_data.dup_loc[0] = &XEXP (XEXP (XEXP (pat, 1), 1), 0);
      recog_data.dup_num[0] = 1;
      recog_data.dup_loc[1] = &XEXP (XEXP (XEXP (pat, 1), 1), 1);
      recog_data.dup_num[1] = 2;
      break;

    case 1625:  /* ssse3_phsubdv2si3 */
    case 1616:  /* ssse3_phadddv2si3 */
    case 1188:  /* sse3_hsubv2df3 */
    case 1187:  /* sse3_haddv2df3 */
    case 983:  /* mmx_hsubv2sf3 */
    case 982:  /* mmx_haddv2sf3 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 0), 0));
      recog_data.dup_loc[0] = &XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 1), 0);
      recog_data.dup_num[0] = 1;
      recog_data.dup_loc[1] = &XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 1), 0);
      recog_data.dup_num[1] = 2;
      break;

    case 946:  /* *lwp_lwpinsdi3_1 */
    case 945:  /* *lwp_lwpinssi3_1 */
      ro[0] = *(ro_loc[0] = &XVECEXP (XEXP (pat, 1), 0, 0));
      ro[1] = *(ro_loc[1] = &XVECEXP (XEXP (pat, 1), 0, 1));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (pat, 1), 0, 2));
      break;

    case 1610:  /* sse3_monitor64 */
    case 1609:  /* sse3_monitor */
    case 944:  /* *lwp_lwpvaldi3_1 */
    case 943:  /* *lwp_lwpvalsi3_1 */
      ro[0] = *(ro_loc[0] = &XVECEXP (pat, 0, 0));
      ro[1] = *(ro_loc[1] = &XVECEXP (pat, 0, 1));
      ro[2] = *(ro_loc[2] = &XVECEXP (pat, 0, 2));
      break;

    case 938:  /* *rdtscp_rex64 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XVECEXP (pat, 0, 2), 0));
      break;

    case 937:  /* *rdtscp */
    case 936:  /* *rdtsc_rex64 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 1), 0));
      break;

    case 934:  /* *rdpmc_rex64 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 0));
      break;

    case 919:  /* *prefetch_sse */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (pat, 2));
      break;

    case 915:  /* adjust_stack_and_probedi */
    case 914:  /* adjust_stack_and_probesi */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 1));
      break;

    case 1242:  /* *fma_fmadd_v4df */
    case 1241:  /* *fma_fmadd_v8sf */
    case 1240:  /* *fma_fmadd_v2df */
    case 1239:  /* *fma_fmadd_v4sf */
    case 1238:  /* *fma_fmadd_df */
    case 1237:  /* *fma_fmadd_sf */
    case 899:  /* *xop_pcmov_df */
    case 898:  /* *xop_pcmov_sf */
    case 897:  /* *movsfcc_1_387 */
    case 896:  /* *movdfcc_1 */
    case 895:  /* *movdfcc_1_rex64 */
    case 894:  /* *movxfcc_1 */
    case 893:  /* *movqicc_noc */
    case 892:  /* *movdicc_noc */
    case 891:  /* *movsicc_noc */
    case 890:  /* *movhicc_noc */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (pat, 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (pat, 1), 1));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (pat, 1), 2));
      break;

    case 883:  /* *strlenqi_1 */
    case 882:  /* *strlenqi_1 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 1));
      ro[3] = *(ro_loc[3] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 2));
      ro[4] = *(ro_loc[4] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 3));
      ro[5] = *(ro_loc[5] = &XEXP (XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 0), 0));
      break;

    case 881:  /* *cmpstrnqi_1 */
    case 880:  /* *cmpstrnqi_1 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 3), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 4), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XVECEXP (pat, 0, 5), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[4] = *(ro_loc[4] = &XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 0), 0));
      ro[5] = *(ro_loc[5] = &XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 1), 0));
      ro[6] = *(ro_loc[6] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0));
      break;

    case 879:  /* *cmpstrnqi_nz_1 */
    case 878:  /* *cmpstrnqi_nz_1 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 3), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 4), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XVECEXP (pat, 0, 5), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XVECEXP (pat, 0, 2), 0));
      ro[4] = *(ro_loc[4] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0));
      ro[5] = *(ro_loc[5] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 0));
      ro[6] = *(ro_loc[6] = &XEXP (XVECEXP (pat, 0, 1), 0));
      break;

    case 877:  /* *rep_stosqi */
    case 876:  /* *rep_stosqi */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XVECEXP (pat, 0, 3), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0));
      ro[4] = *(ro_loc[4] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 1));
      recog_data.dup_loc[0] = &XEXP (XVECEXP (pat, 0, 4), 0);
      recog_data.dup_num[0] = 4;
      recog_data.dup_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 2), 0), 0);
      recog_data.dup_num[1] = 3;
      break;

    case 875:  /* *rep_stossi */
    case 874:  /* *rep_stossi */
    case 873:  /* *rep_stosdi_rex64 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XVECEXP (pat, 0, 3), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 1));
      ro[4] = *(ro_loc[4] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0), 0));
      recog_data.dup_loc[0] = &XEXP (XVECEXP (pat, 0, 4), 0);
      recog_data.dup_num[0] = 4;
      recog_data.dup_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 2), 0), 0);
      recog_data.dup_num[1] = 3;
      break;

    case 872:  /* *strsetqi_1 */
    case 871:  /* *strsetqi_1 */
    case 870:  /* *strsethi_1 */
    case 869:  /* *strsethi_1 */
    case 868:  /* *strsetsi_1 */
    case 867:  /* *strsetsi_1 */
    case 866:  /* *strsetdi_rex_1 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XVECEXP (pat, 0, 0), 1));
      recog_data.dup_loc[0] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0);
      recog_data.dup_num[0] = 1;
      break;

    case 865:  /* *rep_movqi */
    case 864:  /* *rep_movqi */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 2), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0));
      ro[4] = *(ro_loc[4] = &XEXP (XEXP (XVECEXP (pat, 0, 2), 1), 0));
      ro[5] = *(ro_loc[5] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 1));
      recog_data.dup_loc[0] = &XEXP (XVECEXP (pat, 0, 4), 0);
      recog_data.dup_num[0] = 5;
      recog_data.dup_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 3), 0), 0);
      recog_data.dup_num[1] = 3;
      recog_data.dup_loc[2] = &XEXP (XEXP (XVECEXP (pat, 0, 3), 1), 0);
      recog_data.dup_num[2] = 4;
      recog_data.dup_loc[3] = &XEXP (XEXP (XVECEXP (pat, 0, 2), 1), 1);
      recog_data.dup_num[3] = 5;
      break;

    case 863:  /* *rep_movsi */
    case 862:  /* *rep_movsi */
    case 861:  /* *rep_movdi_rex64 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 2), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 1));
      ro[4] = *(ro_loc[4] = &XEXP (XEXP (XVECEXP (pat, 0, 2), 1), 1));
      ro[5] = *(ro_loc[5] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0), 0));
      recog_data.dup_loc[0] = &XEXP (XVECEXP (pat, 0, 4), 0);
      recog_data.dup_num[0] = 5;
      recog_data.dup_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 3), 0), 0);
      recog_data.dup_num[1] = 3;
      recog_data.dup_loc[2] = &XEXP (XEXP (XVECEXP (pat, 0, 3), 1), 0);
      recog_data.dup_num[2] = 4;
      recog_data.dup_loc[3] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 2), 1), 0), 0);
      recog_data.dup_num[3] = 5;
      break;

    case 860:  /* *strmovqi_1 */
    case 859:  /* *strmovqi_1 */
    case 858:  /* *strmovhi_1 */
    case 857:  /* *strmovhi_1 */
    case 856:  /* *strmovsi_1 */
    case 855:  /* *strmovsi_1 */
    case 854:  /* *strmovdi_rex_1 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 2), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 0), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0));
      recog_data.dup_loc[0] = &XEXP (XEXP (XVECEXP (pat, 0, 2), 1), 0);
      recog_data.dup_num[0] = 3;
      recog_data.dup_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0);
      recog_data.dup_num[1] = 2;
      break;

    case 838:  /* fistdi2_ceil_with_temp */
    case 827:  /* fistdi2_floor_with_temp */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 0));
      ro[2] = *(ro_loc[2] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XVECEXP (pat, 0, 2), 0));
      ro[4] = *(ro_loc[4] = &XEXP (XVECEXP (pat, 0, 3), 0));
      ro[5] = *(ro_loc[5] = &XEXP (XVECEXP (pat, 0, 4), 0));
      break;

    case 842:  /* fistsi2_ceil_with_temp */
    case 841:  /* fisthi2_ceil_with_temp */
    case 837:  /* fistdi2_ceil */
    case 831:  /* fistsi2_floor_with_temp */
    case 830:  /* fisthi2_floor_with_temp */
    case 826:  /* fistdi2_floor */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 0));
      ro[2] = *(ro_loc[2] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XVECEXP (pat, 0, 2), 0));
      ro[4] = *(ro_loc[4] = &XEXP (XVECEXP (pat, 0, 3), 0));
      break;

    case 913:  /* allocate_stack_worker_probe_di */
    case 912:  /* allocate_stack_worker_probe_si */
    case 845:  /* frndintxf2_mask_pm */
    case 843:  /* frndintxf2_trunc */
    case 836:  /* *fistdi2_ceil_1 */
    case 835:  /* *fistsi2_ceil_1 */
    case 834:  /* *fisthi2_ceil_1 */
    case 832:  /* frndintxf2_ceil */
    case 825:  /* *fistdi2_floor_1 */
    case 824:  /* *fistsi2_floor_1 */
    case 823:  /* *fisthi2_floor_1 */
    case 821:  /* frndintxf2_floor */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 0));
      break;

    case 2017:  /* atomic_storedi_fpu */
    case 2012:  /* atomic_loaddi_fpu */
    case 846:  /* frndintxf2_mask_pm_i387 */
    case 844:  /* frndintxf2_trunc_i387 */
    case 840:  /* fistsi2_ceil */
    case 839:  /* fisthi2_ceil */
    case 833:  /* frndintxf2_ceil_i387 */
    case 829:  /* fistsi2_floor */
    case 828:  /* fisthi2_floor */
    case 822:  /* frndintxf2_floor_i387 */
    case 814:  /* fistdi2_with_temp */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 0));
      ro[2] = *(ro_loc[2] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XVECEXP (pat, 0, 2), 0));
      break;

    case 924:  /* stack_tls_protect_set_di */
    case 923:  /* stack_tls_protect_set_si */
    case 922:  /* stack_protect_set_di */
    case 921:  /* stack_protect_set_si */
    case 820:  /* fistsi2_with_temp */
    case 819:  /* fisthi2_with_temp */
    case 813:  /* fistdi2 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 0));
      ro[2] = *(ro_loc[2] = &XEXP (XVECEXP (pat, 0, 1), 0));
      break;

    case 808:  /* *fscalexf4_i387 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 0));
      ro[3] = *(ro_loc[3] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 1));
      recog_data.dup_loc[0] = &XVECEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0, 1);
      recog_data.dup_num[0] = 3;
      recog_data.dup_loc[1] = &XVECEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0, 0);
      recog_data.dup_num[1] = 2;
      break;

    case 803:  /* fyl2xp1_extenddfxf3_i387 */
    case 802:  /* fyl2xp1_extendsfxf3_i387 */
    case 800:  /* fyl2x_extenddfxf3_i387 */
    case 799:  /* fyl2x_extendsfxf3_i387 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 0), 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 1));
      ro[3] = *(ro_loc[3] = &XEXP (XVECEXP (pat, 0, 1), 0));
      break;

    case 797:  /* fpatan_extenddfxf3_i387 */
    case 796:  /* fpatan_extendsfxf3_i387 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 1), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XVECEXP (pat, 0, 1), 0));
      break;

    case 928:  /* stack_tls_protect_test_di */
    case 927:  /* stack_tls_protect_test_si */
    case 926:  /* stack_protect_test_di */
    case 925:  /* stack_protect_test_si */
    case 801:  /* fyl2xp1xf3_i387 */
    case 798:  /* fyl2xxf3_i387 */
    case 795:  /* *fpatanxf3_i387 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 1));
      ro[3] = *(ro_loc[3] = &XEXP (XVECEXP (pat, 0, 1), 0));
      break;

    case 794:  /* fptan_extenddfxf4_i387 */
    case 793:  /* fptan_extendsfxf4_i387 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XVECEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0, 0), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XVECEXP (pat, 0, 0), 1));
      break;

    case 792:  /* fptanxf4_i387 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0, 0));
      ro[3] = *(ro_loc[3] = &XEXP (XVECEXP (pat, 0, 0), 1));
      break;

    case 806:  /* fxtract_extenddfxf3_i387 */
    case 805:  /* fxtract_extendsfxf3_i387 */
    case 791:  /* sincos_extenddfxf3_i387 */
    case 790:  /* sincos_extendsfxf3_i387 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 0), 0));
      recog_data.dup_loc[0] = &XEXP (XVECEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0, 0), 0);
      recog_data.dup_num[0] = 2;
      break;

    case 804:  /* fxtractxf3_i387 */
    case 789:  /* sincosxf3 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 0));
      recog_data.dup_loc[0] = &XVECEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0, 0);
      recog_data.dup_num[0] = 2;
      break;

    case 782:  /* fprem1xf4_i387 */
    case 781:  /* fpremxf4_i387 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 0));
      ro[3] = *(ro_loc[3] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 1));
      recog_data.dup_loc[0] = &XVECEXP (XEXP (XVECEXP (pat, 0, 2), 1), 0, 1);
      recog_data.dup_num[0] = 3;
      recog_data.dup_loc[1] = &XVECEXP (XEXP (XVECEXP (pat, 0, 2), 1), 0, 0);
      recog_data.dup_num[1] = 2;
      recog_data.dup_loc[2] = &XVECEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0, 1);
      recog_data.dup_num[2] = 3;
      recog_data.dup_loc[3] = &XVECEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0, 0);
      recog_data.dup_num[3] = 2;
      break;

    case 772:  /* *fop_xf_6_i387 */
    case 771:  /* *fop_xf_6_i387 */
    case 760:  /* *fop_df_6_i387 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (pat, 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (pat, 1), 1), 0));
      ro[3] = *(ro_loc[3] = &XEXP (pat, 1));
      break;

    case 770:  /* *fop_xf_5_i387 */
    case 769:  /* *fop_xf_5_i387 */
    case 766:  /* *fop_xf_3_i387 */
    case 765:  /* *fop_xf_3_i387 */
    case 759:  /* *fop_df_5_i387 */
    case 757:  /* *fop_df_3_i387 */
    case 756:  /* *fop_sf_3_i387 */
    case 755:  /* *fop_df_3_i387 */
    case 754:  /* *fop_sf_3_i387 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (pat, 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (pat, 1), 1), 0));
      ro[3] = *(ro_loc[3] = &XEXP (pat, 1));
      break;

    case 768:  /* *fop_xf_4_i387 */
    case 767:  /* *fop_xf_4_i387 */
    case 764:  /* *fop_xf_2_i387 */
    case 763:  /* *fop_xf_2_i387 */
    case 758:  /* *fop_df_4_i387 */
    case 753:  /* *fop_df_2_i387 */
    case 752:  /* *fop_sf_2_i387 */
    case 751:  /* *fop_df_2_i387 */
    case 750:  /* *fop_sf_2_i387 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (pat, 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (pat, 1), 1));
      ro[3] = *(ro_loc[3] = &XEXP (pat, 1));
      break;

    case 736:  /* *tls_dynamic_gnu2_combine_64 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XVECEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 0), 0, 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0, 0));
      ro[3] = *(ro_loc[3] = &XVECEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0, 1));
      break;

    case 2019:  /* storedi_via_fpu */
    case 2018:  /* loaddi_via_fpu */
    case 1932:  /* vcvtph2ps256 */
    case 1931:  /* *vcvtph2ps_load */
    case 1913:  /* avx_pd256_pd */
    case 1912:  /* avx_ps256_ps */
    case 1911:  /* avx_si256_si */
    case 1831:  /* aesimc */
    case 1804:  /* xop_frczv4df2 */
    case 1803:  /* xop_frczv8sf2 */
    case 1802:  /* xop_frczv2df2 */
    case 1801:  /* xop_frczv4sf2 */
    case 1800:  /* xop_frczdf2 */
    case 1799:  /* xop_frczsf2 */
    case 1691:  /* sse4_1_phminposuw */
    case 1680:  /* sse4_1_movntdqa */
    case 1679:  /* avx2_movntdqa */
    case 1660:  /* sse4a_movntdf */
    case 1659:  /* sse4a_movntsf */
    case 1602:  /* sse2_pmovmskb */
    case 1601:  /* avx2_pmovmskb */
    case 1600:  /* sse2_movmskpd */
    case 1599:  /* avx_movmskpd256 */
    case 1598:  /* sse_movmskps */
    case 1597:  /* avx_movmskps256 */
    case 1316:  /* avx_cvtpd2dq256 */
    case 1310:  /* sse2_cvtsd2siq_2 */
    case 1308:  /* sse2_cvtsd2si_2 */
    case 1303:  /* sse2_cvtpd2pi */
    case 1299:  /* sse2_cvtps2dq */
    case 1298:  /* avx_cvtps2dq256 */
    case 1293:  /* sse_cvtss2siq_2 */
    case 1291:  /* sse_cvtss2si_2 */
    case 1151:  /* sse_rsqrtv4sf2 */
    case 1150:  /* avx_rsqrtv8sf2 */
    case 1142:  /* sse_rcpv4sf2 */
    case 1141:  /* avx_rcpv8sf2 */
    case 1112:  /* sse2_movntv2di */
    case 1111:  /* avx_movntv4di */
    case 1110:  /* sse2_movntv2df */
    case 1109:  /* avx_movntv4df */
    case 1108:  /* sse_movntv4sf */
    case 1107:  /* avx_movntv8sf */
    case 1106:  /* sse2_movntidi */
    case 1105:  /* sse2_movntisi */
    case 1104:  /* sse3_lddqu */
    case 1103:  /* avx_lddqu256 */
    case 1102:  /* sse2_storedqu */
    case 1101:  /* avx_storedqu256 */
    case 1100:  /* sse2_loaddqu */
    case 1099:  /* avx_loaddqu256 */
    case 1098:  /* sse2_storeupd */
    case 1097:  /* avx_storeupd256 */
    case 1096:  /* sse_storeups */
    case 1095:  /* avx_storeups256 */
    case 1094:  /* sse2_loadupd */
    case 1093:  /* avx_loadupd256 */
    case 1092:  /* sse_loadups */
    case 1091:  /* avx_loadups256 */
    case 1070:  /* mmx_pmovmskb */
    case 980:  /* mmx_rsqrtv2sf2 */
    case 977:  /* mmx_rcpv2sf2 */
    case 969:  /* sse_movntq */
    case 933:  /* *rdpmc */
    case 852:  /* movmsk_df */
    case 851:  /* fxamdf2_i387_with_temp */
    case 850:  /* fxamsf2_i387_with_temp */
    case 849:  /* fxamxf2_i387 */
    case 848:  /* fxamdf2_i387 */
    case 847:  /* fxamsf2_i387 */
    case 818:  /* fistsi2 */
    case 817:  /* fisthi2 */
    case 816:  /* *fistsi2_1 */
    case 815:  /* *fisthi2_1 */
    case 812:  /* *fistdi2_1 */
    case 811:  /* rintxf2 */
    case 807:  /* *f2xm1xf2_i387 */
    case 786:  /* *cosxf2_i387 */
    case 783:  /* *sinxf2_i387 */
    case 778:  /* *rsqrtsf2_sse */
    case 774:  /* truncxfdf2_i387_noop_unspec */
    case 773:  /* truncxfsf2_i387_noop_unspec */
    case 745:  /* *rcpsf2_sse */
    case 734:  /* *tls_dynamic_gnu2_lea_64 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XVECEXP (XEXP (pat, 1), 0, 0));
      break;

    case 733:  /* *tls_dynamic_gnu2_combine_32 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XVECEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 0), 0, 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0, 2));
      ro[3] = *(ro_loc[3] = &XVECEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0, 0));
      ro[4] = *(ro_loc[4] = &XVECEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0, 1));
      break;

    case 732:  /* *tls_dynamic_gnu2_call_32 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 1));
      ro[3] = *(ro_loc[3] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 2));
      break;

    case 731:  /* *tls_dynamic_gnu2_lea_32 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (pat, 1), 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (XEXP (XEXP (pat, 1), 1), 0), 0, 0));
      break;

    case 728:  /* *add_tp_x32_zext */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 1));
      break;

    case 730:  /* *add_tp_di */
    case 729:  /* *add_tp_si */
    case 727:  /* *add_tp_x32 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1));
      break;

    case 722:  /* *tls_local_dynamic_32_once */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XVECEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0, 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0, 1));
      ro[3] = *(ro_loc[3] = &XVECEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 0), 0, 0));
      ro[4] = *(ro_loc[4] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[5] = *(ro_loc[5] = &XEXP (XVECEXP (pat, 0, 2), 0));
      break;

    case 720:  /* *tls_local_dynamic_base_32_gnu */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 1));
      ro[3] = *(ro_loc[3] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[4] = *(ro_loc[4] = &XEXP (XVECEXP (pat, 0, 2), 0));
      break;

    case 719:  /* *tls_global_dynamic_64 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XVECEXP (XVECEXP (pat, 0, 1), 0, 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1));
      break;

    case 718:  /* *tls_global_dynamic_32_gnu */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 1));
      ro[3] = *(ro_loc[3] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 2));
      ro[4] = *(ro_loc[4] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[5] = *(ro_loc[5] = &XEXP (XVECEXP (pat, 0, 2), 0));
      break;

    case 717:  /* *parityhi2_cmp */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[1] = *(ro_loc[1] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 0));
      break;

    case 716:  /* paritysi2_cmp */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 2), 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 0));
      break;

    case 715:  /* paritydi2_cmp */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 2), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XVECEXP (pat, 0, 3), 0));
      ro[3] = *(ro_loc[3] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 0));
      break;

    case 714:  /* bswaphi_lowpart */
    case 713:  /* *bswaphi_lowpart_1 */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 0), 0));
      recog_data.dup_loc[0] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0);
      recog_data.dup_num[0] = 0;
      break;

    case 701:  /* *bsrhi */
    case 700:  /* bsr */
    case 699:  /* bsr_rex64 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 0));
      break;

    case 698:  /* *tbm_tzmsk_di */
    case 697:  /* *tbm_tzmsk_si */
    case 696:  /* *tbm_t1mskc_di */
    case 695:  /* *tbm_t1mskc_si */
    case 694:  /* *tbm_blsic_di */
    case 693:  /* *tbm_blsic_si */
    case 686:  /* *tbm_blcic_di */
    case 685:  /* *tbm_blcic_si */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0));
      recog_data.dup_loc[0] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 0);
      recog_data.dup_num[0] = 1;
      break;

    case 684:  /* *tbm_blci_di */
    case 683:  /* *tbm_blci_si */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0), 0));
      recog_data.dup_loc[0] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1);
      recog_data.dup_num[0] = 1;
      break;

    case 680:  /* tbm_bextri_di */
    case 679:  /* tbm_bextri_si */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 2));
      break;

    case 2016:  /* atomic_storedi_1 */
    case 2015:  /* atomic_storesi_1 */
    case 2014:  /* atomic_storehi_1 */
    case 2013:  /* atomic_storeqi_1 */
    case 1935:  /* vcvtps2ph256 */
    case 1934:  /* *vcvtps2ph_store */
    case 1874:  /* avx_vpermilvarv2df3 */
    case 1873:  /* avx_vpermilvarv4df3 */
    case 1872:  /* avx_vpermilvarv4sf3 */
    case 1871:  /* avx_vpermilvarv8sf3 */
    case 1846:  /* avx2_permvarv8sf */
    case 1845:  /* avx2_permv4df */
    case 1844:  /* avx2_permvarv8si */
    case 1832:  /* aeskeygenassist */
    case 1830:  /* aesdeclast */
    case 1829:  /* aesdec */
    case 1828:  /* aesenclast */
    case 1827:  /* aesenc */
    case 1725:  /* sse4_1_roundpd */
    case 1724:  /* avx_roundpd256 */
    case 1723:  /* sse4_1_roundps */
    case 1722:  /* avx_roundps256 */
    case 1666:  /* sse4a_insertq */
    case 1664:  /* sse4a_extrq */
    case 1646:  /* ssse3_psignv2si3 */
    case 1645:  /* ssse3_psignv4hi3 */
    case 1644:  /* ssse3_psignv8qi3 */
    case 1643:  /* ssse3_psignv4si3 */
    case 1642:  /* avx2_psignv8si3 */
    case 1641:  /* ssse3_psignv8hi3 */
    case 1640:  /* avx2_psignv16hi3 */
    case 1639:  /* ssse3_psignv16qi3 */
    case 1638:  /* avx2_psignv32qi3 */
    case 1637:  /* ssse3_pshufbv8qi3 */
    case 1636:  /* ssse3_pshufbv16qi3 */
    case 1635:  /* avx2_pshufbv32qi3 */
    case 1596:  /* sse2_psadbw */
    case 1595:  /* avx2_psadbw */
    case 1180:  /* *ieee_smaxv2df3 */
    case 1179:  /* *ieee_smaxv4df3 */
    case 1178:  /* *ieee_smaxv4sf3 */
    case 1177:  /* *ieee_smaxv8sf3 */
    case 1176:  /* *ieee_sminv2df3 */
    case 1175:  /* *ieee_sminv4df3 */
    case 1174:  /* *ieee_sminv4sf3 */
    case 1173:  /* *ieee_sminv8sf3 */
    case 1069:  /* mmx_psadbw */
    case 981:  /* mmx_rsqit1v2sf3 */
    case 979:  /* mmx_rcpit2v2sf3 */
    case 978:  /* mmx_rcpit1v2sf3 */
    case 932:  /* sse4_2_crc32di */
    case 931:  /* sse4_2_crc32si */
    case 930:  /* sse4_2_crc32hi */
    case 929:  /* sse4_2_crc32qi */
    case 907:  /* *ieee_smaxdf3 */
    case 906:  /* *ieee_smaxsf3 */
    case 905:  /* *ieee_smindf3 */
    case 904:  /* *ieee_sminsf3 */
    case 810:  /* sse4_1_rounddf2 */
    case 809:  /* sse4_1_roundsf2 */
    case 678:  /* bmi2_pext_di3 */
    case 677:  /* bmi2_pext_si3 */
    case 676:  /* bmi2_pdep_di3 */
    case 675:  /* bmi2_pdep_si3 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XVECEXP (XEXP (pat, 1), 0, 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (pat, 1), 0, 1));
      break;

    case 674:  /* bmi2_bzhi_di3 */
    case 673:  /* bmi2_bzhi_si3 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 1));
      break;

    case 692:  /* *tbm_blsfill_di */
    case 691:  /* *tbm_blsfill_si */
    case 690:  /* *tbm_blcs_di */
    case 689:  /* *tbm_blcs_si */
    case 688:  /* *tbm_blcmsk_di */
    case 687:  /* *tbm_blcmsk_si */
    case 682:  /* *tbm_blcfill_di */
    case 681:  /* *tbm_blcfill_si */
    case 672:  /* *bmi_blsr_di */
    case 671:  /* *bmi_blsr_si */
    case 670:  /* *bmi_blsmsk_di */
    case 669:  /* *bmi_blsmsk_si */
    case 668:  /* *bmi_blsi_di */
    case 667:  /* *bmi_blsi_si */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0));
      recog_data.dup_loc[0] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1);
      recog_data.dup_num[0] = 1;
      break;

    case 917:  /* probe_stack_rangedi */
    case 916:  /* probe_stack_rangesi */
    case 735:  /* *tls_dynamic_gnu2_call_64 */
    case 666:  /* bmi_bextr_di */
    case 665:  /* bmi_bextr_si */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 1));
      break;

    case 656:  /* *ffsdi_1 */
    case 655:  /* *ffssi_1 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0));
      recog_data.dup_loc[0] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0);
      recog_data.dup_num[0] = 1;
      break;

    case 1662:  /* sse4a_vmmovntv2df */
    case 1661:  /* sse4a_vmmovntv4sf */
    case 1309:  /* sse2_cvtsd2siq */
    case 1307:  /* sse2_cvtsd2si */
    case 1292:  /* sse_cvtss2siq */
    case 1290:  /* sse_cvtss2si */
    case 788:  /* *cos_extenddfxf2_i387 */
    case 787:  /* *cos_extendsfxf2_i387 */
    case 785:  /* *sin_extenddfxf2_i387 */
    case 784:  /* *sin_extendsfxf2_i387 */
    case 649:  /* set_got_offset_rex64 */
    case 648:  /* set_rip_rex64 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (XEXP (pat, 1), 0, 0), 0));
      break;

    case 646:  /* set_got_labelled */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 0), 0));
      break;

    case 957:  /* rdranddi_1 */
    case 956:  /* rdrandsi_1 */
    case 955:  /* rdrandhi_1 */
    case 645:  /* set_got */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      break;

    case 641:  /* simple_return_indirect_internal */
    case 640:  /* simple_return_pop_internal */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 1), 0));
      break;

    case 1835:  /* avx_vzeroupper */
    case 1607:  /* sse2_clflush */
    case 1605:  /* sse_ldmxcsr */
    case 954:  /* wrgsbasedi */
    case 953:  /* wrgsbasesi */
    case 952:  /* wrfsbasedi */
    case 951:  /* wrfsbasesi */
    case 940:  /* *lwp_llwpcbdi1 */
    case 939:  /* *lwp_llwpcbsi1 */
    case 653:  /* split_stack_return */
    case 644:  /* pad */
    case 643:  /* nops */
    case 637:  /* prologue_use */
      ro[0] = *(ro_loc[0] = &XVECEXP (pat, 0, 0));
      break;

    case 2010:  /* mfence_sse2 */
    case 2009:  /* *sse_sfence */
    case 2008:  /* *sse2_lfence */
    case 958:  /* *pause */
    case 636:  /* *memory_blockage */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      recog_data.dup_loc[0] = &XVECEXP (XEXP (pat, 1), 0, 0);
      recog_data.dup_num[0] = 0;
      break;

    case 918:  /* trap */
    case 853:  /* cld */
    case 652:  /* leave_rex64 */
    case 651:  /* leave */
    case 650:  /* eh_return_internal */
    case 642:  /* nop */
    case 639:  /* simple_return_internal_long */
    case 638:  /* simple_return_internal */
    case 635:  /* blockage */
      break;

    case 634:  /* *sibcall_value_pop */
    case 632:  /* *call_value_pop */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 1));
      break;

    case 633:  /* *sibcall_value_pop_vzeroupper */
    case 631:  /* *call_value_pop_vzeroupper */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 1));
      ro[4] = *(ro_loc[4] = &XVECEXP (XVECEXP (pat, 0, 2), 0, 0));
      break;

    case 629:  /* *call_value_rex64_ms_sysv_vzeroupper */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1));
      ro[3] = *(ro_loc[3] = &XVECEXP (XVECEXP (pat, 0, 14), 0, 0));
      break;

    case 1894:  /* vec_set_hi_v32qi */
    case 1892:  /* vec_set_hi_v16hi */
    case 1890:  /* vec_set_hi_v8sf */
    case 1889:  /* vec_set_hi_v8si */
    case 1886:  /* vec_set_hi_v4df */
    case 1885:  /* vec_set_hi_v4di */
    case 1882:  /* avx2_vec_set_hi_v4di */
    case 1535:  /* *andnottf3 */
    case 1510:  /* *andnotv2di3 */
    case 1509:  /* *andnotv4di3 */
    case 1508:  /* *andnotv4si3 */
    case 1507:  /* *andnotv8si3 */
    case 1506:  /* *andnotv8hi3 */
    case 1505:  /* *andnotv16hi3 */
    case 1504:  /* *andnotv16qi3 */
    case 1503:  /* *andnotv32qi3 */
    case 1387:  /* sse2_loadhpd */
    case 1343:  /* sse_loadhps */
    case 1325:  /* *sse2_cvtpd2ps */
    case 1321:  /* *sse2_cvttpd2dq */
    case 1320:  /* *avx_cvttpd2dq256_2 */
    case 1230:  /* *andnotdf3 */
    case 1229:  /* *andnotsf3 */
    case 1216:  /* sse2_andnotv2df3 */
    case 1215:  /* avx_andnotv4df3 */
    case 1214:  /* sse_andnotv4sf3 */
    case 1213:  /* avx_andnotv8sf3 */
    case 1149:  /* sse2_vmsqrtv2df2 */
    case 1148:  /* sse_vmsqrtv4sf2 */
    case 1039:  /* mmx_andnotv2si3 */
    case 1038:  /* mmx_andnotv4hi3 */
    case 1037:  /* mmx_andnotv8qi3 */
    case 628:  /* *sibcall_value */
    case 627:  /* *sibcall_value */
    case 624:  /* *call_value */
    case 623:  /* *call_value */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (pat, 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (pat, 1), 1));
      break;

    case 626:  /* *sibcall_value_vzeroupper */
    case 625:  /* *sibcall_value_vzeroupper */
    case 622:  /* *call_value_vzeroupper */
    case 621:  /* *call_value_vzeroupper */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1));
      ro[3] = *(ro_loc[3] = &XVECEXP (XVECEXP (pat, 0, 1), 0, 0));
      break;

    case 620:  /* *sibcall_pop */
    case 618:  /* *call_pop */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 0), 1));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 1));
      break;

    case 619:  /* *sibcall_pop_vzeroupper */
    case 617:  /* *call_pop_vzeroupper */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 0), 1));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 1));
      ro[3] = *(ro_loc[3] = &XVECEXP (XVECEXP (pat, 0, 2), 0, 0));
      break;

    case 611:  /* *call_rex64_ms_sysv_vzeroupper */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 0), 1));
      ro[2] = *(ro_loc[2] = &XVECEXP (XVECEXP (pat, 0, 14), 0, 0));
      break;

    case 614:  /* *sibcall_vzeroupper */
    case 613:  /* *sibcall_vzeroupper */
    case 608:  /* *call_vzeroupper */
    case 607:  /* *call_vzeroupper */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 0), 1));
      ro[2] = *(ro_loc[2] = &XVECEXP (XVECEXP (pat, 0, 1), 0, 0));
      break;

    case 606:  /* *tablejump_1 */
    case 605:  /* *tablejump_1 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 1));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 0), 0));
      break;

    case 604:  /* *indirect_jump */
    case 603:  /* *indirect_jump */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 1));
      break;

    case 602:  /* jump */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (pat, 1), 0));
      break;

    case 601:  /* *fp_jcc_4_si_387 */
    case 600:  /* *fp_jcc_4_hi_387 */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 1));
      ro[4] = *(ro_loc[4] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 0));
      ro[5] = *(ro_loc[5] = &XEXP (XVECEXP (pat, 0, 3), 0));
      break;

    case 598:  /* *fp_jcc_2r_387 */
    case 596:  /* *fp_jcc_1r_387 */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 1));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 2), 0));
      ro[4] = *(ro_loc[4] = &XEXP (XVECEXP (pat, 0, 3), 0));
      break;

    case 599:  /* *fp_jcc_3_387 */
    case 597:  /* *fp_jcc_2_387 */
    case 595:  /* *fp_jcc_1_387 */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 1));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 0));
      ro[4] = *(ro_loc[4] = &XEXP (XVECEXP (pat, 0, 3), 0));
      break;

    case 594:  /* *jcc_btsi_mask_1 */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0), 0), 1), 0), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0), 0), 1), 0), 1));
      ro[4] = *(ro_loc[4] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 0));
      break;

    case 593:  /* *jcc_btsi_1 */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0), 0), 1));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 0));
      break;

    case 592:  /* *jcc_btdi_mask */
    case 591:  /* *jcc_btsi_mask */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0), 2), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0), 2), 1));
      ro[4] = *(ro_loc[4] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 0));
      break;

    case 590:  /* *jcc_btdi */
    case 589:  /* *jcc_btsi */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0), 2), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 0));
      break;

    case 588:  /* *jcc_2 */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (XEXP (pat, 1), 2), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (pat, 1), 0));
      break;

    case 587:  /* *jcc_1 */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (XEXP (pat, 1), 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (pat, 1), 0));
      break;

    case 1206:  /* sse2_maskcmpv2df3 */
    case 1205:  /* avx_maskcmpv4df3 */
    case 1204:  /* sse_maskcmpv4sf3 */
    case 1203:  /* avx_maskcmpv8sf3 */
    case 1202:  /* *sse2_maskcmpv2df3_comm */
    case 1201:  /* *avx_maskcmpv4df3_comm */
    case 1200:  /* *sse_maskcmpv4sf3_comm */
    case 1199:  /* *avx_maskcmpv8sf3_comm */
    case 762:  /* *fop_xf_1_i387 */
    case 761:  /* *fop_xf_comm_i387 */
    case 749:  /* *fop_df_1_i387 */
    case 748:  /* *fop_sf_1_i387 */
    case 747:  /* *fop_df_1_sse */
    case 746:  /* *fop_sf_1_sse */
    case 744:  /* *fop_df_1_mixed */
    case 743:  /* *fop_sf_1_mixed */
    case 742:  /* *fop_df_comm_i387 */
    case 741:  /* *fop_sf_comm_i387 */
    case 740:  /* *fop_df_comm_sse */
    case 739:  /* *fop_sf_comm_sse */
    case 738:  /* *fop_df_comm_mixed */
    case 737:  /* *fop_sf_comm_mixed */
    case 586:  /* setcc_df_sse */
    case 585:  /* setcc_sf_sse */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (pat, 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (pat, 1), 1));
      ro[3] = *(ro_loc[3] = &XEXP (pat, 1));
      break;

    case 579:  /* *btdi */
    case 578:  /* *btsi */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (XEXP (pat, 1), 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (pat, 1), 0), 2));
      break;

    case 577:  /* *btcq */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 0), 2));
      recog_data.dup_loc[0] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0);
      recog_data.dup_num[0] = 0;
      recog_data.dup_loc[1] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 2);
      recog_data.dup_num[1] = 1;
      break;

    case 576:  /* *btrq */
    case 575:  /* *btsq */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 0), 2));
      break;

    case 559:  /* ix86_rotrti3_doubleword */
    case 558:  /* ix86_rotrdi3_doubleword */
    case 557:  /* ix86_rotlti3_doubleword */
    case 556:  /* ix86_rotldi3_doubleword */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1));
      ro[3] = *(ro_loc[3] = &XEXP (XVECEXP (pat, 0, 2), 0));
      break;

    case 1556:  /* vec_interleave_lowv4si */
    case 1555:  /* avx2_interleave_lowv8si */
    case 1554:  /* vec_interleave_highv4si */
    case 1553:  /* avx2_interleave_highv8si */
    case 1552:  /* vec_interleave_lowv8hi */
    case 1551:  /* avx2_interleave_lowv16hi */
    case 1550:  /* vec_interleave_highv8hi */
    case 1549:  /* avx2_interleave_highv16hi */
    case 1548:  /* vec_interleave_lowv16qi */
    case 1547:  /* avx2_interleave_lowv32qi */
    case 1546:  /* vec_interleave_highv16qi */
    case 1545:  /* avx2_interleave_highv32qi */
    case 1380:  /* vec_interleave_lowv2di */
    case 1379:  /* avx2_interleave_lowv4di */
    case 1378:  /* vec_interleave_highv2di */
    case 1377:  /* avx2_interleave_highv4di */
    case 1375:  /* *vec_interleave_lowv2df */
    case 1374:  /* *avx_unpcklpd256 */
    case 1373:  /* *vec_interleave_highv2df */
    case 1372:  /* avx_unpckhpd256 */
    case 1334:  /* vec_interleave_lowv4sf */
    case 1333:  /* avx_unpcklps256 */
    case 1332:  /* vec_interleave_highv4sf */
    case 1331:  /* avx_unpckhps256 */
    case 1330:  /* sse_movlhps */
    case 1329:  /* sse_movhlps */
    case 1057:  /* mmx_punpckldq */
    case 1056:  /* mmx_punpckhdq */
    case 1055:  /* mmx_punpcklwd */
    case 1054:  /* mmx_punpckhwd */
    case 1053:  /* mmx_punpcklbw */
    case 1052:  /* mmx_punpckhbw */
    case 566:  /* *bmi2_rorxsi3_1_zext */
    case 525:  /* *bmi2_ashrsi3_1_zext */
    case 524:  /* *bmi2_lshrsi3_1_zext */
    case 489:  /* *bmi2_ashlsi3_1_zext */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (pat, 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (pat, 1), 0), 1));
      break;

    case 1929:  /* avx_vec_concatv4df */
    case 1928:  /* avx_vec_concatv8sf */
    case 1927:  /* avx_vec_concatv4di */
    case 1926:  /* avx_vec_concatv8si */
    case 1925:  /* avx_vec_concatv16hi */
    case 1924:  /* avx_vec_concatv32qi */
    case 1923:  /* avx2_lshrvv2di */
    case 1922:  /* avx2_ashlvv2di */
    case 1921:  /* avx2_lshrvv4di */
    case 1920:  /* avx2_ashlvv4di */
    case 1919:  /* avx2_lshrvv4si */
    case 1918:  /* avx2_ashlvv4si */
    case 1917:  /* avx2_lshrvv8si */
    case 1916:  /* avx2_ashlvv8si */
    case 1915:  /* avx2_ashrvv4si */
    case 1914:  /* avx2_ashrvv8si */
    case 1786:  /* xop_rotrv2di3 */
    case 1785:  /* xop_rotrv4si3 */
    case 1784:  /* xop_rotrv8hi3 */
    case 1783:  /* xop_rotrv16qi3 */
    case 1782:  /* xop_rotlv2di3 */
    case 1781:  /* xop_rotlv4si3 */
    case 1780:  /* xop_rotlv8hi3 */
    case 1779:  /* xop_rotlv16qi3 */
    case 1590:  /* vec_concatv2di */
    case 1589:  /* *vec_concatv2di_rex64 */
    case 1588:  /* *vec_concatv4si */
    case 1587:  /* *vec_concatv2si_sse */
    case 1586:  /* *vec_concatv2si_sse2 */
    case 1585:  /* *vec_concatv2si_sse4_1 */
    case 1538:  /* *xortf3 */
    case 1537:  /* *iortf3 */
    case 1536:  /* *andtf3 */
    case 1534:  /* *xorv2di3 */
    case 1533:  /* *iorv2di3 */
    case 1532:  /* *andv2di3 */
    case 1531:  /* *xorv4di3 */
    case 1530:  /* *iorv4di3 */
    case 1529:  /* *andv4di3 */
    case 1528:  /* *xorv4si3 */
    case 1527:  /* *iorv4si3 */
    case 1526:  /* *andv4si3 */
    case 1525:  /* *xorv8si3 */
    case 1524:  /* *iorv8si3 */
    case 1523:  /* *andv8si3 */
    case 1522:  /* *xorv8hi3 */
    case 1521:  /* *iorv8hi3 */
    case 1520:  /* *andv8hi3 */
    case 1519:  /* *xorv16hi3 */
    case 1518:  /* *iorv16hi3 */
    case 1517:  /* *andv16hi3 */
    case 1516:  /* *xorv16qi3 */
    case 1515:  /* *iorv16qi3 */
    case 1514:  /* *andv16qi3 */
    case 1513:  /* *xorv32qi3 */
    case 1512:  /* *iorv32qi3 */
    case 1511:  /* *andv32qi3 */
    case 1502:  /* sse2_gtv4si3 */
    case 1501:  /* sse2_gtv8hi3 */
    case 1500:  /* sse2_gtv16qi3 */
    case 1499:  /* avx2_gtv4di3 */
    case 1498:  /* avx2_gtv8si3 */
    case 1497:  /* avx2_gtv16hi3 */
    case 1496:  /* avx2_gtv32qi3 */
    case 1495:  /* sse4_2_gtv2di3 */
    case 1494:  /* *sse2_eqv4si3 */
    case 1493:  /* *sse2_eqv8hi3 */
    case 1492:  /* *sse2_eqv16qi3 */
    case 1491:  /* *sse4_1_eqv2di3 */
    case 1490:  /* *avx2_eqv4di3 */
    case 1489:  /* *avx2_eqv8si3 */
    case 1488:  /* *avx2_eqv16hi3 */
    case 1487:  /* *avx2_eqv32qi3 */
    case 1486:  /* *uminv16qi3 */
    case 1485:  /* *umaxv16qi3 */
    case 1484:  /* *sse4_1_uminv4si3 */
    case 1483:  /* *sse4_1_umaxv4si3 */
    case 1482:  /* *sse4_1_uminv8hi3 */
    case 1481:  /* *sse4_1_umaxv8hi3 */
    case 1480:  /* *sminv8hi3 */
    case 1479:  /* *smaxv8hi3 */
    case 1478:  /* *sse4_1_sminv4si3 */
    case 1477:  /* *sse4_1_smaxv4si3 */
    case 1476:  /* *sse4_1_sminv16qi3 */
    case 1475:  /* *sse4_1_smaxv16qi3 */
    case 1474:  /* *avx2_uminv8si3 */
    case 1473:  /* *avx2_umaxv8si3 */
    case 1472:  /* *avx2_sminv8si3 */
    case 1471:  /* *avx2_smaxv8si3 */
    case 1470:  /* *avx2_uminv16hi3 */
    case 1469:  /* *avx2_umaxv16hi3 */
    case 1468:  /* *avx2_sminv16hi3 */
    case 1467:  /* *avx2_smaxv16hi3 */
    case 1466:  /* *avx2_uminv32qi3 */
    case 1465:  /* *avx2_umaxv32qi3 */
    case 1464:  /* *avx2_sminv32qi3 */
    case 1463:  /* *avx2_smaxv32qi3 */
    case 1462:  /* sse2_lshrv1ti3 */
    case 1461:  /* avx2_lshrv2ti3 */
    case 1460:  /* sse2_ashlv1ti3 */
    case 1459:  /* avx2_ashlv2ti3 */
    case 1458:  /* lshrv2di3 */
    case 1457:  /* ashlv2di3 */
    case 1456:  /* lshrv4di3 */
    case 1455:  /* ashlv4di3 */
    case 1454:  /* lshrv4si3 */
    case 1453:  /* ashlv4si3 */
    case 1452:  /* lshrv8si3 */
    case 1451:  /* ashlv8si3 */
    case 1450:  /* lshrv8hi3 */
    case 1449:  /* ashlv8hi3 */
    case 1448:  /* lshrv16hi3 */
    case 1447:  /* ashlv16hi3 */
    case 1446:  /* ashrv4si3 */
    case 1445:  /* ashrv8si3 */
    case 1444:  /* ashrv8hi3 */
    case 1443:  /* ashrv16hi3 */
    case 1442:  /* mulv2di3 */
    case 1441:  /* mulv4di3 */
    case 1440:  /* *sse2_mulv4si3 */
    case 1439:  /* *sse4_1_mulv4si3 */
    case 1438:  /* *avx2_mulv8si3 */
    case 1427:  /* *mulv8hi3 */
    case 1426:  /* *mulv16hi3 */
    case 1425:  /* mulv16qi3 */
    case 1424:  /* mulv32qi3 */
    case 1423:  /* *sse2_ussubv8hi3 */
    case 1422:  /* *sse2_sssubv8hi3 */
    case 1421:  /* *sse2_usaddv8hi3 */
    case 1420:  /* *sse2_ssaddv8hi3 */
    case 1419:  /* *avx2_ussubv16hi3 */
    case 1418:  /* *avx2_sssubv16hi3 */
    case 1417:  /* *avx2_usaddv16hi3 */
    case 1416:  /* *avx2_ssaddv16hi3 */
    case 1415:  /* *sse2_ussubv16qi3 */
    case 1414:  /* *sse2_sssubv16qi3 */
    case 1413:  /* *sse2_usaddv16qi3 */
    case 1412:  /* *sse2_ssaddv16qi3 */
    case 1411:  /* *avx2_ussubv32qi3 */
    case 1410:  /* *avx2_sssubv32qi3 */
    case 1409:  /* *avx2_usaddv32qi3 */
    case 1408:  /* *avx2_ssaddv32qi3 */
    case 1407:  /* *subv2di3 */
    case 1406:  /* *addv2di3 */
    case 1405:  /* *subv4di3 */
    case 1404:  /* *addv4di3 */
    case 1403:  /* *subv4si3 */
    case 1402:  /* *addv4si3 */
    case 1401:  /* *subv8si3 */
    case 1400:  /* *addv8si3 */
    case 1399:  /* *subv8hi3 */
    case 1398:  /* *addv8hi3 */
    case 1397:  /* *subv16hi3 */
    case 1396:  /* *addv16hi3 */
    case 1395:  /* *subv16qi3 */
    case 1394:  /* *addv16qi3 */
    case 1393:  /* *subv32qi3 */
    case 1392:  /* *addv32qi3 */
    case 1391:  /* *vec_concatv2df */
    case 1352:  /* *vec_concatv4sf */
    case 1351:  /* *vec_concatv2sf_sse */
    case 1350:  /* *vec_concatv2sf_sse4_1 */
    case 1236:  /* *xordf3 */
    case 1235:  /* *iordf3 */
    case 1234:  /* *anddf3 */
    case 1233:  /* *xorsf3 */
    case 1232:  /* *iorsf3 */
    case 1231:  /* *andsf3 */
    case 1228:  /* *xorv2df3 */
    case 1227:  /* *iorv2df3 */
    case 1226:  /* *andv2df3 */
    case 1225:  /* *xorv4df3 */
    case 1224:  /* *iorv4df3 */
    case 1223:  /* *andv4df3 */
    case 1222:  /* *xorv4sf3 */
    case 1221:  /* *iorv4sf3 */
    case 1220:  /* *andv4sf3 */
    case 1219:  /* *xorv8sf3 */
    case 1218:  /* *iorv8sf3 */
    case 1217:  /* *andv8sf3 */
    case 1168:  /* *sminv2df3 */
    case 1167:  /* *smaxv2df3 */
    case 1166:  /* *sminv4df3 */
    case 1165:  /* *smaxv4df3 */
    case 1164:  /* *sminv4sf3 */
    case 1163:  /* *smaxv4sf3 */
    case 1162:  /* *sminv8sf3 */
    case 1161:  /* *smaxv8sf3 */
    case 1160:  /* *sminv2df3_finite */
    case 1159:  /* *smaxv2df3_finite */
    case 1158:  /* *sminv4df3_finite */
    case 1157:  /* *smaxv4df3_finite */
    case 1156:  /* *sminv4sf3_finite */
    case 1155:  /* *smaxv4sf3_finite */
    case 1154:  /* *sminv8sf3_finite */
    case 1153:  /* *smaxv8sf3_finite */
    case 1138:  /* sse2_divv2df3 */
    case 1137:  /* avx_divv4df3 */
    case 1136:  /* sse_divv4sf3 */
    case 1135:  /* avx_divv8sf3 */
    case 1132:  /* *mulv2df3 */
    case 1131:  /* *mulv4df3 */
    case 1130:  /* *mulv4sf3 */
    case 1129:  /* *mulv8sf3 */
    case 1124:  /* *subv2df3 */
    case 1123:  /* *addv2df3 */
    case 1122:  /* *subv4df3 */
    case 1121:  /* *addv4df3 */
    case 1120:  /* *subv4sf3 */
    case 1119:  /* *addv4sf3 */
    case 1118:  /* *subv8sf3 */
    case 1117:  /* *addv8sf3 */
    case 1064:  /* *mmx_concatv2si */
    case 1048:  /* *mmx_xorv2si3 */
    case 1047:  /* *mmx_iorv2si3 */
    case 1046:  /* *mmx_andv2si3 */
    case 1045:  /* *mmx_xorv4hi3 */
    case 1044:  /* *mmx_iorv4hi3 */
    case 1043:  /* *mmx_andv4hi3 */
    case 1042:  /* *mmx_xorv8qi3 */
    case 1041:  /* *mmx_iorv8qi3 */
    case 1040:  /* *mmx_andv8qi3 */
    case 1036:  /* mmx_gtv2si3 */
    case 1035:  /* mmx_gtv4hi3 */
    case 1034:  /* mmx_gtv8qi3 */
    case 1033:  /* *mmx_eqv2si3 */
    case 1032:  /* *mmx_eqv4hi3 */
    case 1031:  /* *mmx_eqv8qi3 */
    case 1030:  /* mmx_lshrv1di3 */
    case 1029:  /* mmx_ashlv1di3 */
    case 1028:  /* mmx_lshrv2si3 */
    case 1027:  /* mmx_ashlv2si3 */
    case 1026:  /* mmx_lshrv4hi3 */
    case 1025:  /* mmx_ashlv4hi3 */
    case 1024:  /* mmx_ashrv2si3 */
    case 1023:  /* mmx_ashrv4hi3 */
    case 1022:  /* *mmx_uminv8qi3 */
    case 1021:  /* *mmx_umaxv8qi3 */
    case 1020:  /* *mmx_sminv4hi3 */
    case 1019:  /* *mmx_smaxv4hi3 */
    case 1013:  /* *mmx_mulv4hi3 */
    case 1012:  /* *mmx_ussubv4hi3 */
    case 1011:  /* *mmx_sssubv4hi3 */
    case 1010:  /* *mmx_usaddv4hi3 */
    case 1009:  /* *mmx_ssaddv4hi3 */
    case 1008:  /* *mmx_ussubv8qi3 */
    case 1007:  /* *mmx_sssubv8qi3 */
    case 1006:  /* *mmx_usaddv8qi3 */
    case 1005:  /* *mmx_ssaddv8qi3 */
    case 1004:  /* *mmx_subv1di3 */
    case 1003:  /* *mmx_addv1di3 */
    case 1002:  /* *mmx_subv2si3 */
    case 1001:  /* *mmx_addv2si3 */
    case 1000:  /* *mmx_subv4hi3 */
    case 999:  /* *mmx_addv4hi3 */
    case 998:  /* *mmx_subv8qi3 */
    case 997:  /* *mmx_addv8qi3 */
    case 994:  /* *mmx_concatv2sf */
    case 987:  /* mmx_gev2sf3 */
    case 986:  /* mmx_gtv2sf3 */
    case 985:  /* *mmx_eqv2sf3 */
    case 976:  /* *mmx_sminv2sf3 */
    case 975:  /* *mmx_smaxv2sf3 */
    case 974:  /* *mmx_sminv2sf3_finite */
    case 973:  /* *mmx_smaxv2sf3_finite */
    case 972:  /* *mmx_mulv2sf3 */
    case 971:  /* *mmx_subv2sf3 */
    case 970:  /* *mmx_addv2sf3 */
    case 903:  /* smindf3 */
    case 902:  /* smaxdf3 */
    case 901:  /* sminsf3 */
    case 900:  /* smaxsf3 */
    case 561:  /* *bmi2_rorxdi3_1 */
    case 560:  /* *bmi2_rorxsi3_1 */
    case 519:  /* *bmi2_ashrdi3_1 */
    case 518:  /* *bmi2_lshrdi3_1 */
    case 517:  /* *bmi2_ashrsi3_1 */
    case 516:  /* *bmi2_lshrsi3_1 */
    case 486:  /* *bmi2_ashldi3_1 */
    case 485:  /* *bmi2_ashlsi3_1 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (pat, 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (pat, 1), 1));
      break;

    case 555:  /* *rotrdi3_mask */
    case 554:  /* *rotldi3_mask */
    case 553:  /* *rotrsi3_mask */
    case 552:  /* *rotlsi3_mask */
    case 506:  /* *ashrdi3_mask */
    case 505:  /* *lshrdi3_mask */
    case 504:  /* *ashrsi3_mask */
    case 503:  /* *lshrsi3_mask */
    case 484:  /* *ashldi3_mask */
    case 483:  /* *ashlsi3_mask */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 0), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 0), 1));
      break;

    case 512:  /* x86_shrd */
    case 511:  /* x86_64_shrd */
    case 482:  /* x86_shld */
    case 481:  /* x86_64_shld */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 1));
      recog_data.dup_loc[0] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0);
      recog_data.dup_num[0] = 0;
      recog_data.dup_loc[1] = &XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 1), 1);
      recog_data.dup_num[1] = 2;
      break;

    case 708:  /* *popcountsi2_cmp_zext */
    case 478:  /* *one_cmplsi2_2_zext */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0));
      recog_data.dup_loc[0] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0), 0);
      recog_data.dup_num[0] = 1;
      break;

    case 468:  /* copysigntf3_var */
    case 467:  /* copysigndf3_var */
    case 466:  /* copysignsf3_var */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 0));
      ro[3] = *(ro_loc[3] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 1));
      ro[4] = *(ro_loc[4] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 2));
      ro[5] = *(ro_loc[5] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 3));
      break;

    case 1877:  /* *avx_vperm2f128v4df_full */
    case 1876:  /* *avx_vperm2f128v8sf_full */
    case 1875:  /* *avx_vperm2f128v8si_full */
    case 1848:  /* avx2_permv2ti */
    case 1833:  /* pclmulqdq */
    case 1822:  /* xop_pcom_tfv2di3 */
    case 1821:  /* xop_pcom_tfv4si3 */
    case 1820:  /* xop_pcom_tfv8hi3 */
    case 1819:  /* xop_pcom_tfv16qi3 */
    case 1775:  /* xop_pperm */
    case 1686:  /* sse4_1_pblendvb */
    case 1685:  /* avx2_pblendvb */
    case 1682:  /* sse4_1_mpsadbw */
    case 1681:  /* avx2_mpsadbw */
    case 1678:  /* sse4_1_dppd */
    case 1677:  /* avx_dppd256 */
    case 1676:  /* sse4_1_dpps */
    case 1675:  /* avx_dpps256 */
    case 1674:  /* sse4_1_blendvpd */
    case 1673:  /* avx_blendvpd256 */
    case 1672:  /* sse4_1_blendvps */
    case 1671:  /* avx_blendvps256 */
    case 1663:  /* sse4a_extrqi */
    case 1649:  /* ssse3_palignrdi */
    case 1648:  /* ssse3_palignrti */
    case 1647:  /* avx2_palignrv2ti */
    case 1264:  /* *fma_fmaddsub_v2df */
    case 1263:  /* *fma_fmaddsub_v4df */
    case 1262:  /* *fma_fmaddsub_v4sf */
    case 1261:  /* *fma_fmaddsub_v8sf */
    case 1196:  /* avx_cmpv2df3 */
    case 1195:  /* avx_cmpv4df3 */
    case 1194:  /* avx_cmpv4sf3 */
    case 1193:  /* avx_cmpv8sf3 */
    case 465:  /* copysigntf3_const */
    case 464:  /* copysigndf3_const */
    case 463:  /* copysignsf3_const */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XVECEXP (XEXP (pat, 1), 0, 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (pat, 1), 0, 1));
      ro[3] = *(ro_loc[3] = &XVECEXP (XEXP (pat, 1), 0, 2));
      break;

    case 1116:  /* *absnegv2df2 */
    case 1115:  /* *absnegv4df2 */
    case 1114:  /* *absnegv4sf2 */
    case 1113:  /* *absnegv8sf2 */
    case 450:  /* *absnegtf2_sse */
    case 449:  /* *absnegxf2_i387 */
    case 448:  /* *absnegdf2_i387 */
    case 447:  /* *absnegsf2_i387 */
    case 446:  /* *absnegdf2_sse */
    case 445:  /* *absnegsf2_sse */
    case 444:  /* *absnegdf2_mixed */
    case 443:  /* *absnegsf2_mixed */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XVECEXP (pat, 0, 0), 1));
      break;

    case 442:  /* *negsi2_cmpz_zext */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0), 0), 0));
      recog_data.dup_loc[0] = &XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0), 0), 0);
      recog_data.dup_num[0] = 1;
      break;

    case 707:  /* *popcountdi2_cmp */
    case 706:  /* *popcountsi2_cmp */
    case 705:  /* *popcounthi2_cmp */
    case 477:  /* *one_cmpldi2_2 */
    case 476:  /* *one_cmplsi2_2 */
    case 475:  /* *one_cmplhi2_2 */
    case 474:  /* *one_cmplqi2_2 */
    case 441:  /* *negdi2_cmpz */
    case 440:  /* *negsi2_cmpz */
    case 439:  /* *neghi2_cmpz */
    case 438:  /* *negqi2_cmpz */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0));
      recog_data.dup_loc[0] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0);
      recog_data.dup_num[0] = 1;
      break;

    case 437:  /* *negsi2_1_zext */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0), 0));
      break;

    case 410:  /* *xorsi_2_zext_imm */
    case 409:  /* *iorsi_2_zext_imm */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 1));
      recog_data.dup_loc[0] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0), 0);
      recog_data.dup_num[0] = 1;
      recog_data.dup_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 1);
      recog_data.dup_num[1] = 2;
      break;

    case 721:  /* *tls_local_dynamic_base_64 */
    case 664:  /* *bmi_andn_di */
    case 663:  /* *bmi_andn_si */
    case 630:  /* *call_value_rex64_ms_sysv */
    case 396:  /* *xorsi_1_zext_imm */
    case 395:  /* *iorsi_1_zext_imm */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1));
      break;

    case 430:  /* *xorqi_cc_ext_1 */
    case 429:  /* *xorqi_cc_ext_1_rex64 */
    case 381:  /* *andqi_ext_0_cc */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 1));
      recog_data.dup_loc[0] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0), 0);
      recog_data.dup_num[0] = 1;
      recog_data.dup_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 1);
      recog_data.dup_num[1] = 2;
      break;

    case 412:  /* *xorqi_2_slp */
    case 411:  /* *iorqi_2_slp */
    case 379:  /* *andqi_2_slp */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 1));
      recog_data.dup_loc[0] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 0), 0);
      recog_data.dup_num[0] = 0;
      recog_data.dup_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0);
      recog_data.dup_num[1] = 0;
      recog_data.dup_loc[2] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 1);
      recog_data.dup_num[2] = 1;
      break;

    case 366:  /* *testqi_ext_3 */
    case 365:  /* *testqi_ext_3_rex64 */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (XEXP (pat, 1), 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (pat, 1), 0), 1));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (pat, 1), 0), 2));
      break;

    case 364:  /* *testqi_ext_2 */
    case 363:  /* *testqi_ext_1 */
    case 362:  /* *testqi_ext_1_rex64 */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 1), 0));
      break;

    case 361:  /* *testqi_ext_0 */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (pat, 1), 0), 1));
      break;

    case 355:  /* udivmodhiqi3 */
    case 346:  /* divmodhiqi3 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0), 0), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0), 0), 0), 1), 0));
      recog_data.dup_loc[0] = &XEXP (XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 0), 0), 0);
      recog_data.dup_num[0] = 1;
      recog_data.dup_loc[1] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 0), 0), 1), 0);
      recog_data.dup_num[1] = 2;
      break;

    case 354:  /* *udivmoddi4_noext */
    case 353:  /* *udivmodsi4_noext */
    case 352:  /* *udivmodhi4_noext */
    case 345:  /* *divmoddi4_noext */
    case 344:  /* *divmodsi4_noext */
    case 343:  /* *divmodhi4_noext */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1));
      ro[4] = *(ro_loc[4] = &XEXP (XVECEXP (pat, 0, 2), 0));
      recog_data.dup_loc[0] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0);
      recog_data.dup_num[0] = 2;
      recog_data.dup_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 1);
      recog_data.dup_num[1] = 3;
      break;

    case 351:  /* *udivmoddi4 */
    case 350:  /* *udivmodsi4 */
    case 349:  /* *udivmodhi4 */
    case 348:  /* udivmoddi4_1 */
    case 347:  /* udivmodsi4_1 */
    case 342:  /* *divmoddi4 */
    case 341:  /* *divmodsi4 */
    case 340:  /* *divmodhi4 */
    case 339:  /* divmoddi4_1 */
    case 338:  /* divmodsi4_1 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1));
      recog_data.dup_loc[0] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0);
      recog_data.dup_num[0] = 2;
      recog_data.dup_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 1);
      recog_data.dup_num[1] = 3;
      break;

    case 337:  /* *umulsi3_highpart_zext */
    case 336:  /* *smulsi3_highpart_zext */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0), 0), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0), 0), 1), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XVECEXP (pat, 0, 1), 0));
      break;

    case 335:  /* *umulsi3_highpart_1 */
    case 334:  /* *smulsi3_highpart_1 */
    case 333:  /* *umuldi3_highpart_1 */
    case 332:  /* *smuldi3_highpart_1 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0), 1), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XVECEXP (pat, 0, 1), 0));
      break;

    case 331:  /* *umulqihi3_1 */
    case 330:  /* *mulqihi3_1 */
    case 329:  /* *mulditi3_1 */
    case 328:  /* *mulsidi3_1 */
    case 327:  /* *umulditi3_1 */
    case 326:  /* *umulsidi3_1 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 0));
      break;

    case 325:  /* *bmi2_umulsidi3_1 */
    case 324:  /* *bmi2_umulditi3_1 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1));
      recog_data.dup_loc[0] = &XEXP (XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0), 0), 0), 0);
      recog_data.dup_num[0] = 2;
      recog_data.dup_loc[1] = &XEXP (XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0), 0), 1), 0);
      recog_data.dup_num[1] = 3;
      break;

    case 318:  /* *addsi3_zext_cc_overflow */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 1));
      recog_data.dup_loc[0] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0), 0);
      recog_data.dup_num[0] = 1;
      recog_data.dup_loc[1] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0), 1);
      recog_data.dup_num[1] = 2;
      recog_data.dup_loc[2] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1);
      recog_data.dup_num[2] = 1;
      break;

    case 317:  /* *adddi3_cc_overflow */
    case 316:  /* *addsi3_cc_overflow */
    case 315:  /* *addhi3_cc_overflow */
    case 314:  /* *addqi3_cc_overflow */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 1));
      recog_data.dup_loc[0] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0);
      recog_data.dup_num[0] = 1;
      recog_data.dup_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 1);
      recog_data.dup_num[1] = 2;
      recog_data.dup_loc[2] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1);
      recog_data.dup_num[2] = 1;
      break;

    case 313:  /* *adddi3_cconly_overflow */
    case 312:  /* *addsi3_cconly_overflow */
    case 311:  /* *addhi3_cconly_overflow */
    case 310:  /* *addqi3_cconly_overflow */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 1));
      recog_data.dup_loc[0] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1);
      recog_data.dup_num[0] = 1;
      break;

    case 309:  /* *subsi3_carry_zext */
    case 308:  /* *addsi3_carry_zext */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 1), 1));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 1), 0));
      break;

    case 307:  /* *subdi3_carry */
    case 306:  /* *adddi3_carry */
    case 305:  /* *subsi3_carry */
    case 304:  /* *addsi3_carry */
    case 303:  /* *subhi3_carry */
    case 302:  /* *addhi3_carry */
    case 301:  /* *subqi3_carry */
    case 300:  /* *addqi3_carry */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 1));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 0));
      break;

    case 299:  /* *subsi_3_zext */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1));
      recog_data.dup_loc[0] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0), 0);
      recog_data.dup_num[0] = 1;
      recog_data.dup_loc[1] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0), 1);
      recog_data.dup_num[1] = 2;
      break;

    case 298:  /* *subdi_3 */
    case 297:  /* *subsi_3 */
    case 296:  /* *subhi_3 */
    case 295:  /* *subqi_3 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1));
      recog_data.dup_loc[0] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0);
      recog_data.dup_num[0] = 1;
      recog_data.dup_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 1);
      recog_data.dup_num[1] = 2;
      break;

    case 279:  /* *lea_general_3 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 1));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XEXP (pat, 1), 0), 1));
      ro[4] = *(ro_loc[4] = &XEXP (XEXP (pat, 1), 1));
      break;

    case 1739:  /* xop_pmacssdd */
    case 1738:  /* xop_pmacsdd */
    case 1737:  /* xop_pmacssww */
    case 1736:  /* xop_pmacsww */
    case 281:  /* *lea_general_4 */
    case 280:  /* *lea_general_4 */
    case 278:  /* *lea_general_2 */
    case 277:  /* *lea_general_1 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (pat, 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (pat, 1), 0), 1));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (pat, 1), 1));
      break;

    case 428:  /* *xorqi_ext_2 */
    case 427:  /* *iorqi_ext_2 */
    case 426:  /* *xorqi_ext_1 */
    case 425:  /* *iorqi_ext_1 */
    case 424:  /* *xorqi_ext_1_rex64 */
    case 423:  /* *iorqi_ext_1_rex64 */
    case 384:  /* *andqi_ext_2 */
    case 383:  /* *andqi_ext_1 */
    case 382:  /* *andqi_ext_1_rex64 */
    case 276:  /* *addqi_ext_2 */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 0));
      break;

    case 422:  /* *xorqi_ext_0 */
    case 421:  /* *iorqi_ext_0 */
    case 380:  /* andqi_ext_0 */
    case 275:  /* addqi_ext_1 */
    case 274:  /* *addqi_ext_1_rex64 */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1));
      break;

    case 551:  /* *ashrdi3_cconly */
    case 550:  /* *lshrdi3_cconly */
    case 549:  /* *ashrsi3_cconly */
    case 548:  /* *lshrsi3_cconly */
    case 547:  /* *ashrhi3_cconly */
    case 546:  /* *lshrhi3_cconly */
    case 545:  /* *ashrqi3_cconly */
    case 544:  /* *lshrqi3_cconly */
    case 502:  /* *ashldi3_cconly */
    case 501:  /* *ashlsi3_cconly */
    case 500:  /* *ashlhi3_cconly */
    case 499:  /* *ashlqi3_cconly */
    case 420:  /* *xordi_3 */
    case 419:  /* *iordi_3 */
    case 418:  /* *xorsi_3 */
    case 417:  /* *iorsi_3 */
    case 416:  /* *xorhi_3 */
    case 415:  /* *iorhi_3 */
    case 414:  /* *xorqi_3 */
    case 413:  /* *iorqi_3 */
    case 273:  /* *adddi_5 */
    case 272:  /* *addsi_5 */
    case 271:  /* *addhi_5 */
    case 270:  /* *addqi_5 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 1));
      break;

    case 265:  /* *addsi_3_zext */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0));
      recog_data.dup_loc[0] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0), 0);
      recog_data.dup_num[0] = 1;
      recog_data.dup_loc[1] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0), 1);
      recog_data.dup_num[1] = 2;
      break;

    case 264:  /* *adddi_3 */
    case 263:  /* *addsi_3 */
    case 262:  /* *addhi_3 */
    case 261:  /* *addqi_3 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0));
      break;

    case 543:  /* *ashrsi3_cmp_zext */
    case 542:  /* *lshrsi3_cmp_zext */
    case 498:  /* *ashlsi3_cmp_zext */
    case 408:  /* *xorsi_2_zext */
    case 407:  /* *iorsi_2_zext */
    case 378:  /* *andsi_2_zext */
    case 294:  /* *subsi_2_zext */
    case 260:  /* *addsi_2_zext */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 1));
      recog_data.dup_loc[0] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0), 0);
      recog_data.dup_num[0] = 1;
      recog_data.dup_loc[1] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0), 1);
      recog_data.dup_num[1] = 2;
      break;

    case 541:  /* *ashrdi3_cmp */
    case 540:  /* *lshrdi3_cmp */
    case 539:  /* *ashrsi3_cmp */
    case 538:  /* *lshrsi3_cmp */
    case 537:  /* *ashrhi3_cmp */
    case 536:  /* *lshrhi3_cmp */
    case 535:  /* *ashrqi3_cmp */
    case 534:  /* *lshrqi3_cmp */
    case 497:  /* *ashldi3_cmp */
    case 496:  /* *ashlsi3_cmp */
    case 495:  /* *ashlhi3_cmp */
    case 494:  /* *ashlqi3_cmp */
    case 406:  /* *xordi_2 */
    case 405:  /* *iordi_2 */
    case 404:  /* *xorsi_2 */
    case 403:  /* *iorsi_2 */
    case 402:  /* *xorhi_2 */
    case 401:  /* *iorhi_2 */
    case 400:  /* *xorqi_2 */
    case 399:  /* *iorqi_2 */
    case 377:  /* *andsi_2 */
    case 376:  /* *andhi_2 */
    case 375:  /* *andqi_2 */
    case 374:  /* *andqi_2_maybe_si */
    case 373:  /* *anddi_2 */
    case 293:  /* *subdi_2 */
    case 292:  /* *subsi_2 */
    case 291:  /* *subhi_2 */
    case 290:  /* *subqi_2 */
    case 259:  /* *adddi_2 */
    case 258:  /* *addsi_2 */
    case 257:  /* *addhi_2 */
    case 256:  /* *addqi_2 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 1));
      recog_data.dup_loc[0] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0);
      recog_data.dup_num[0] = 1;
      recog_data.dup_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 1);
      recog_data.dup_num[1] = 2;
      break;

    case 574:  /* *rotrqi3_1_slp */
    case 573:  /* *rotlqi3_1_slp */
    case 533:  /* *ashrqi3_1_slp */
    case 532:  /* *lshrqi3_1_slp */
    case 493:  /* *ashlqi3_1_slp */
    case 398:  /* *xorqi_1_slp */
    case 397:  /* *iorqi_1_slp */
    case 372:  /* *andqi_1_slp */
    case 289:  /* *subqi_1_slp */
    case 255:  /* *addqi_1_slp */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1));
      recog_data.dup_loc[0] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0);
      recog_data.dup_num[0] = 0;
      break;

    case 568:  /* *rotrsi3_1_zext */
    case 567:  /* *rotlsi3_1_zext */
    case 527:  /* *ashrsi3_1_zext */
    case 526:  /* *lshrsi3_1_zext */
    case 515:  /* *ashrsi3_cvt_zext */
    case 490:  /* *ashlsi3_1_zext */
    case 394:  /* *xorsi_1_zext */
    case 393:  /* *iorsi_1_zext */
    case 369:  /* *andsi_1_zext */
    case 321:  /* *mulsi3_1_zext */
    case 288:  /* *subsi_1_zext */
    case 252:  /* addsi_1_zext */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0), 1));
      break;

    case 249:  /* addqi3_cc */
    case 248:  /* *adddi3_cc */
    case 247:  /* *addsi3_cc */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[1] = *(ro_loc[1] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 0));
      ro[2] = *(ro_loc[2] = &XVECEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0, 1));
      recog_data.dup_loc[0] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 0);
      recog_data.dup_num[0] = 1;
      recog_data.dup_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 1), 1), 1);
      recog_data.dup_num[1] = 2;
      break;

    case 911:  /* pro_epilogue_adjust_stack_di_sub */
    case 910:  /* pro_epilogue_adjust_stack_si_sub */
    case 909:  /* pro_epilogue_adjust_stack_di_add */
    case 908:  /* pro_epilogue_adjust_stack_si_add */
    case 572:  /* *rotrhi3_1 */
    case 571:  /* *rotlhi3_1 */
    case 570:  /* *rotrqi3_1 */
    case 569:  /* *rotlqi3_1 */
    case 565:  /* *rotrdi3_1 */
    case 564:  /* *rotldi3_1 */
    case 563:  /* *rotrsi3_1 */
    case 562:  /* *rotlsi3_1 */
    case 531:  /* *ashrhi3_1 */
    case 530:  /* *lshrhi3_1 */
    case 529:  /* *ashrqi3_1 */
    case 528:  /* *lshrqi3_1 */
    case 523:  /* *ashrdi3_1 */
    case 522:  /* *lshrdi3_1 */
    case 521:  /* *ashrsi3_1 */
    case 520:  /* *lshrsi3_1 */
    case 514:  /* ashrsi3_cvt */
    case 513:  /* ashrdi3_cvt */
    case 510:  /* *ashrti3_doubleword */
    case 509:  /* *lshrti3_doubleword */
    case 508:  /* *ashrdi3_doubleword */
    case 507:  /* *lshrdi3_doubleword */
    case 492:  /* *ashlqi3_1 */
    case 491:  /* *ashlhi3_1 */
    case 488:  /* *ashldi3_1 */
    case 487:  /* *ashlsi3_1 */
    case 480:  /* *ashlti3_doubleword */
    case 479:  /* *ashldi3_doubleword */
    case 392:  /* *xorqi_1 */
    case 391:  /* *iorqi_1 */
    case 390:  /* *xordi_1 */
    case 389:  /* *iordi_1 */
    case 388:  /* *xorsi_1 */
    case 387:  /* *iorsi_1 */
    case 386:  /* *xorhi_1 */
    case 385:  /* *iorhi_1 */
    case 371:  /* *andqi_1 */
    case 370:  /* *andhi_1 */
    case 368:  /* *andsi_1 */
    case 367:  /* *anddi_1 */
    case 323:  /* *mulqi3_1 */
    case 322:  /* *mulhi3_1 */
    case 320:  /* *muldi3_1 */
    case 319:  /* *mulsi3_1 */
    case 287:  /* *subdi_1 */
    case 286:  /* *subsi_1 */
    case 285:  /* *subhi_1 */
    case 284:  /* *subqi_1 */
    case 283:  /* *subti3_doubleword */
    case 282:  /* *subdi3_doubleword */
    case 254:  /* *addqi_1 */
    case 253:  /* *addhi_1 */
    case 251:  /* *adddi_1 */
    case 250:  /* *addsi_1 */
    case 246:  /* *addti3_doubleword */
    case 245:  /* *adddi3_doubleword */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1));
      break;

    case 239:  /* floatdixf2_i387_with_xmm */
    case 238:  /* floatdidf2_i387_with_xmm */
    case 237:  /* floatdisf2_i387_with_xmm */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XVECEXP (pat, 0, 3), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[4] = *(ro_loc[4] = &XEXP (XVECEXP (pat, 0, 2), 0));
      break;

    case 171:  /* fix_truncdi_i387_with_temp */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XVECEXP (pat, 0, 2), 0));
      ro[4] = *(ro_loc[4] = &XEXP (XVECEXP (pat, 0, 3), 0));
      ro[5] = *(ro_loc[5] = &XEXP (XVECEXP (pat, 0, 4), 0));
      break;

    case 175:  /* fix_truncsi_i387_with_temp */
    case 174:  /* fix_trunchi_i387_with_temp */
    case 170:  /* fix_truncdi_i387 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XVECEXP (pat, 0, 2), 0));
      ro[4] = *(ro_loc[4] = &XEXP (XVECEXP (pat, 0, 3), 0));
      break;

    case 242:  /* *floatunssixf2_1 */
    case 241:  /* *floatunssidf2_1 */
    case 240:  /* *floatunssisf2_1 */
    case 173:  /* fix_truncsi_i387 */
    case 172:  /* fix_trunchi_i387 */
    case 166:  /* fix_truncdi_i387_fisttp_with_temp */
    case 165:  /* fix_truncsi_i387_fisttp_with_temp */
    case 164:  /* fix_trunchi_i387_fisttp_with_temp */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XVECEXP (pat, 0, 2), 0));
      break;

    case 153:  /* *fixuns_truncdf_1 */
    case 152:  /* *fixuns_truncsf_1 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 2), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XVECEXP (pat, 0, 3), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0));
      ro[4] = *(ro_loc[4] = &XEXP (XVECEXP (pat, 0, 1), 0));
      break;

    case 1090:  /* movdi_to_sse */
    case 654:  /* ffssi2_no_cmove */
    case 230:  /* *floatdixf2_i387_with_temp */
    case 229:  /* *floatdidf2_i387_with_temp */
    case 228:  /* *floatdisf2_i387_with_temp */
    case 227:  /* *floatsixf2_i387_with_temp */
    case 226:  /* *floatsidf2_i387_with_temp */
    case 225:  /* *floatsisf2_i387_with_temp */
    case 216:  /* *floatdidf2_sse_with_temp */
    case 215:  /* *floatdisf2_sse_with_temp */
    case 214:  /* *floatsidf2_sse_with_temp */
    case 213:  /* *floatsisf2_sse_with_temp */
    case 210:  /* *floatsidf2_vector_sse_with_temp */
    case 209:  /* *floatsisf2_vector_sse_with_temp */
    case 200:  /* *floatdidf2_mixed_with_temp */
    case 199:  /* *floatdisf2_mixed_with_temp */
    case 198:  /* *floatsidf2_mixed_with_temp */
    case 197:  /* *floatsisf2_mixed_with_temp */
    case 194:  /* *floatsidf2_vector_mixed_with_temp */
    case 193:  /* *floatsisf2_vector_mixed_with_temp */
    case 183:  /* *floathixf2_i387_with_temp */
    case 182:  /* *floathidf2_i387_with_temp */
    case 181:  /* *floathisf2_i387_with_temp */
    case 163:  /* fix_truncdi_i387_fisttp */
    case 162:  /* fix_truncsi_i387_fisttp */
    case 161:  /* fix_trunchi_i387_fisttp */
    case 147:  /* *truncxfdf2_mixed */
    case 146:  /* *truncxfsf2_mixed */
    case 144:  /* *truncdfsf_i387 */
    case 143:  /* *truncdfsf_mixed */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XVECEXP (pat, 0, 1), 0));
      break;

    case 127:  /* extendsidi2_1 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XVECEXP (pat, 0, 2), 0));
      break;

    case 889:  /* *x86_movdicc_0_m1_neg */
    case 888:  /* *x86_movsicc_0_m1_neg */
    case 887:  /* *x86_movdicc_0_m1_se */
    case 886:  /* *x86_movsicc_0_m1_se */
    case 885:  /* *x86_movdicc_0_m1 */
    case 884:  /* *x86_movsicc_0_m1 */
    case 704:  /* popcountdi2 */
    case 703:  /* popcountsi2 */
    case 702:  /* popcounthi2 */
    case 662:  /* clzdi2_lzcnt */
    case 661:  /* clzsi2_lzcnt */
    case 660:  /* clzhi2_lzcnt */
    case 659:  /* ctzdi2 */
    case 658:  /* ctzsi2 */
    case 657:  /* ctzhi2 */
    case 436:  /* *negdi2_1 */
    case 435:  /* *negsi2_1 */
    case 434:  /* *neghi2_1 */
    case 433:  /* *negqi2_1 */
    case 432:  /* *negti2_doubleword */
    case 431:  /* *negdi2_doubleword */
    case 169:  /* *fix_truncdi_i387_1 */
    case 168:  /* *fix_truncsi_i387_1 */
    case 167:  /* *fix_trunchi_i387_1 */
    case 123:  /* *zero_extendqisi2_movzbl_and */
    case 122:  /* *zero_extendqihi2_movzbl_and */
    case 121:  /* *zero_extendqisi2_and */
    case 120:  /* *zero_extendqihi2_and */
    case 118:  /* zero_extendhisi2_and */
    case 115:  /* zero_extendsidi2_1 */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0));
      break;

    case 96:  /* *movqi_insv_2 */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (pat, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (pat, 1), 0));
      break;

    case 1849:  /* avx2_vec_dupv4df */
    case 1843:  /* avx2_pbroadcastv2di */
    case 1842:  /* avx2_pbroadcastv4di */
    case 1841:  /* avx2_pbroadcastv4si */
    case 1840:  /* avx2_pbroadcastv8si */
    case 1839:  /* avx2_pbroadcastv8hi */
    case 1838:  /* avx2_pbroadcastv16hi */
    case 1837:  /* avx2_pbroadcastv16qi */
    case 1836:  /* avx2_pbroadcastv32qi */
    case 1715:  /* sse4_1_zero_extendv2siv2di2 */
    case 1714:  /* sse4_1_sign_extendv2siv2di2 */
    case 1711:  /* sse4_1_zero_extendv2hiv2di2 */
    case 1710:  /* sse4_1_sign_extendv2hiv2di2 */
    case 1709:  /* avx2_zero_extendv4hiv4di2 */
    case 1708:  /* avx2_sign_extendv4hiv4di2 */
    case 1707:  /* sse4_1_zero_extendv2qiv2di2 */
    case 1706:  /* sse4_1_sign_extendv2qiv2di2 */
    case 1705:  /* avx2_zero_extendv4qiv4di2 */
    case 1704:  /* avx2_sign_extendv4qiv4di2 */
    case 1703:  /* sse4_1_zero_extendv4hiv4si2 */
    case 1702:  /* sse4_1_sign_extendv4hiv4si2 */
    case 1699:  /* sse4_1_zero_extendv4qiv4si2 */
    case 1698:  /* sse4_1_sign_extendv4qiv4si2 */
    case 1697:  /* avx2_zero_extendv8qiv8si2 */
    case 1696:  /* avx2_sign_extendv8qiv8si2 */
    case 1695:  /* sse4_1_zero_extendv8qiv8hi2 */
    case 1694:  /* sse4_1_sign_extendv8qiv8hi2 */
    case 1348:  /* avx2_vec_dupv4sf */
    case 1347:  /* avx2_vec_dupv8sf */
    case 1328:  /* sse2_cvtps2pd */
    case 1327:  /* *avx_cvtps2pd256_2 */
    case 1315:  /* sse2_cvtdq2pd */
    case 1314:  /* avx_cvtdq2pd256_2 */
    case 1312:  /* sse2_cvttsd2siq */
    case 1311:  /* sse2_cvttsd2si */
    case 1295:  /* sse_cvttss2siq */
    case 1294:  /* sse_cvttss2si */
    case 1287:  /* sse_cvttps2pi */
    case 1089:  /* sse2_movq128 */
    case 1062:  /* *vec_dupv4hi */
    case 777:  /* sqrt_extenddfxf2_i387 */
    case 776:  /* sqrt_extendsfxf2_i387 */
    case 473:  /* *one_cmplsi2_1_zext */
    case 462:  /* *negextenddfxf2 */
    case 461:  /* *absextenddfxf2 */
    case 460:  /* *negextendsfxf2 */
    case 459:  /* *absextendsfxf2 */
    case 458:  /* *negextendsfdf2 */
    case 457:  /* *absextendsfdf2 */
    case 133:  /* *extendqisi2_zext */
    case 131:  /* *extendhisi2_zext */
    case 92:  /* *movqi_extzv_2 */
    case 91:  /* *movqi_extzv_2_rex64 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (pat, 1), 0), 0));
      break;

    case 612:  /* *call_rex64_ms_sysv */
    case 84:  /* *movstricthi_xor */
    case 83:  /* *movstrictqi_xor */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 0), 1));
      break;

    case 113:  /* *swapdf */
    case 112:  /* *swapsf */
    case 111:  /* swapxf */
    case 80:  /* *swaphi_2 */
    case 79:  /* *swapqi_2 */
    case 78:  /* *swaphi_1 */
    case 77:  /* *swapqi_1 */
    case 76:  /* *swapdi */
    case 75:  /* *swapsi */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 0), 1));
      recog_data.dup_loc[0] = &XEXP (XVECEXP (pat, 0, 1), 0);
      recog_data.dup_num[0] = 1;
      recog_data.dup_loc[1] = &XEXP (XVECEXP (pat, 0, 1), 1);
      recog_data.dup_num[1] = 0;
      break;

    case 1853:  /* vec_dupv4df */
    case 1852:  /* vec_dupv4di */
    case 1851:  /* vec_dupv8sf */
    case 1850:  /* vec_dupv8si */
    case 1713:  /* avx2_zero_extendv4siv4di2 */
    case 1712:  /* avx2_sign_extendv4siv4di2 */
    case 1701:  /* avx2_zero_extendv8hiv8si2 */
    case 1700:  /* avx2_sign_extendv8hiv8si2 */
    case 1693:  /* avx2_zero_extendv16qiv16hi2 */
    case 1692:  /* avx2_sign_extendv16qiv16hi2 */
    case 1658:  /* absv2si2 */
    case 1657:  /* absv4hi2 */
    case 1656:  /* absv8qi2 */
    case 1655:  /* absv4si2 */
    case 1654:  /* absv8si2 */
    case 1653:  /* absv8hi2 */
    case 1652:  /* absv16hi2 */
    case 1651:  /* absv16qi2 */
    case 1650:  /* absv32qi2 */
    case 1584:  /* *vec_dupv2di */
    case 1583:  /* *vec_dupv4si */
    case 1582:  /* *vec_extractv2di_1 */
    case 1581:  /* *vec_extractv2di_1_rex64 */
    case 1580:  /* *sse2_storeq */
    case 1579:  /* *sse2_storeq_rex64 */
    case 1577:  /* sse2_stored */
    case 1390:  /* vec_dupv2df */
    case 1386:  /* *vec_extractv2df_0_sse */
    case 1385:  /* sse2_storelpd */
    case 1384:  /* *vec_extractv2df_1_sse */
    case 1383:  /* sse2_storehpd */
    case 1371:  /* vec_extract_hi_v32qi */
    case 1370:  /* vec_extract_lo_v32qi */
    case 1369:  /* vec_extract_hi_v16hi */
    case 1368:  /* vec_extract_lo_v16hi */
    case 1367:  /* vec_extract_hi_v8sf */
    case 1366:  /* vec_extract_hi_v8si */
    case 1365:  /* vec_extract_lo_v8sf */
    case 1364:  /* vec_extract_lo_v8si */
    case 1363:  /* vec_extract_hi_v4df */
    case 1362:  /* vec_extract_hi_v4di */
    case 1361:  /* vec_extract_lo_v4df */
    case 1360:  /* vec_extract_lo_v4di */
    case 1357:  /* *vec_extractv4sf_0 */
    case 1349:  /* vec_dupv4sf */
    case 1344:  /* sse_storelps */
    case 1342:  /* sse_storehps */
    case 1326:  /* avx_cvtps2pd256 */
    case 1324:  /* avx_cvtpd2ps256 */
    case 1319:  /* fix_truncv4dfv4si2 */
    case 1313:  /* floatv4siv4df2 */
    case 1304:  /* sse2_cvttpd2pi */
    case 1302:  /* sse2_cvtpi2pd */
    case 1301:  /* fix_truncv4sfv4si2 */
    case 1300:  /* fix_truncv8sfv8si2 */
    case 1297:  /* floatv4siv4sf2 */
    case 1296:  /* floatv8siv8sf2 */
    case 1147:  /* sse2_sqrtv2df2 */
    case 1146:  /* avx_sqrtv4df2 */
    case 1145:  /* sse_sqrtv4sf2 */
    case 1144:  /* avx_sqrtv8sf2 */
    case 1066:  /* *vec_extractv2si_1 */
    case 1065:  /* *vec_extractv2si_0 */
    case 1063:  /* *vec_dupv2si */
    case 1061:  /* mmx_pswapdv2si2 */
    case 996:  /* *vec_extractv2sf_1 */
    case 995:  /* *vec_extractv2sf_0 */
    case 993:  /* *vec_dupv2sf */
    case 992:  /* mmx_pswapdv2sf2 */
    case 991:  /* mmx_floatv2si2 */
    case 988:  /* mmx_pf2id */
    case 780:  /* *sqrtdf2_sse */
    case 779:  /* *sqrtsf2_sse */
    case 775:  /* sqrtxf2 */
    case 712:  /* *bswapdi2_1 */
    case 711:  /* *bswapsi2_1 */
    case 710:  /* *bswapdi2_movbe */
    case 709:  /* *bswapsi2_movbe */
    case 472:  /* *one_cmplqi2_1 */
    case 471:  /* *one_cmpldi2_1 */
    case 470:  /* *one_cmplsi2_1 */
    case 469:  /* *one_cmplhi2_1 */
    case 456:  /* *negxf2_1 */
    case 455:  /* *absxf2_1 */
    case 454:  /* *negdf2_1 */
    case 453:  /* *absdf2_1 */
    case 452:  /* *negsf2_1 */
    case 451:  /* *abssf2_1 */
    case 236:  /* *floatdixf2_i387 */
    case 235:  /* *floatdidf2_i387 */
    case 234:  /* *floatdisf2_i387 */
    case 233:  /* *floatsixf2_i387 */
    case 232:  /* *floatsidf2_i387 */
    case 231:  /* *floatsisf2_i387 */
    case 224:  /* *floatdidf2_sse_nointerunit */
    case 223:  /* *floatdisf2_sse_nointerunit */
    case 222:  /* *floatsidf2_sse_nointerunit */
    case 221:  /* *floatsisf2_sse_nointerunit */
    case 220:  /* *floatdidf2_sse_interunit */
    case 219:  /* *floatdisf2_sse_interunit */
    case 218:  /* *floatsidf2_sse_interunit */
    case 217:  /* *floatsisf2_sse_interunit */
    case 212:  /* *floatsidf2_vector_sse */
    case 211:  /* *floatsisf2_vector_sse */
    case 208:  /* *floatdidf2_mixed_nointerunit */
    case 207:  /* *floatdisf2_mixed_nointerunit */
    case 206:  /* *floatsidf2_mixed_nointerunit */
    case 205:  /* *floatsisf2_mixed_nointerunit */
    case 204:  /* *floatdidf2_mixed_interunit */
    case 203:  /* *floatdisf2_mixed_interunit */
    case 202:  /* *floatsidf2_mixed_interunit */
    case 201:  /* *floatsisf2_mixed_interunit */
    case 196:  /* *floatsidf2_vector_mixed */
    case 195:  /* *floatsisf2_vector_mixed */
    case 192:  /* *floatdixf2_1 */
    case 191:  /* *floatdidf2_1 */
    case 190:  /* *floatdisf2_1 */
    case 189:  /* *floatsixf2_1 */
    case 188:  /* *floatsidf2_1 */
    case 187:  /* *floatsisf2_1 */
    case 186:  /* *floathixf2_i387 */
    case 185:  /* *floathidf2_i387 */
    case 184:  /* *floathisf2_i387 */
    case 180:  /* *floathixf2_1 */
    case 179:  /* *floathidf2_1 */
    case 178:  /* *floathisf2_1 */
    case 160:  /* fix_truncdi_fisttp_i387_1 */
    case 159:  /* fix_truncsi_fisttp_i387_1 */
    case 158:  /* fix_trunchi_fisttp_i387_1 */
    case 157:  /* fix_truncdfsi_sse */
    case 156:  /* fix_truncsfsi_sse */
    case 155:  /* fix_truncdfdi_sse */
    case 154:  /* fix_truncsfdi_sse */
    case 151:  /* *truncxfdf2_i387 */
    case 150:  /* *truncxfsf2_i387 */
    case 149:  /* truncxfdf2_i387_noop */
    case 148:  /* truncxfsf2_i387_noop */
    case 145:  /* *truncdfsf2_i387_1 */
    case 142:  /* *truncdfsf_fast_i387 */
    case 141:  /* *truncdfsf_fast_sse */
    case 140:  /* *truncdfsf_fast_mixed */
    case 139:  /* *extenddfxf2_i387 */
    case 138:  /* *extendsfxf2_i387 */
    case 137:  /* *extendsfdf2_i387 */
    case 136:  /* *extendsfdf2_sse */
    case 135:  /* *extendsfdf2_mixed */
    case 134:  /* extendqihi2 */
    case 132:  /* extendqisi2 */
    case 130:  /* extendhisi2 */
    case 129:  /* extendhidi2 */
    case 128:  /* extendqidi2 */
    case 126:  /* *extendsidi2_rex64 */
    case 125:  /* *zero_extendqisi2_movzbl */
    case 124:  /* *zero_extendqihi2_movzbl */
    case 119:  /* *zero_extendhisi2_movzwl */
    case 117:  /* zero_extendhidi2 */
    case 116:  /* zero_extendqidi2 */
    case 114:  /* *zero_extendsidi2_rex64 */
    case 90:  /* *movdi_extzv_1 */
    case 89:  /* *movsi_extzv_1 */
    case 88:  /* *movqi_extv_1 */
    case 87:  /* *movqi_extv_1_rex64 */
    case 86:  /* *movsi_extv_1 */
    case 85:  /* *movhi_extv_1 */
    case 74:  /* *movabsdi_2 */
    case 73:  /* *movabssi_2 */
    case 72:  /* *movabshi_2 */
    case 71:  /* *movabsqi_2 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (pat, 1), 0));
      break;

    case 616:  /* *sibcall */
    case 615:  /* *sibcall */
    case 610:  /* *call */
    case 609:  /* *call */
    case 584:  /* *setcc_qi_slp */
    case 95:  /* *movsi_insv_1 */
    case 94:  /* *movdi_insv_1_rex64 */
    case 93:  /* *movsi_insv_1_rex64 */
    case 82:  /* *movstricthi_1 */
    case 81:  /* *movstrictqi_1 */
    case 70:  /* *movabsdi_1 */
    case 69:  /* *movabssi_1 */
    case 68:  /* *movabshi_1 */
    case 67:  /* *movabsqi_1 */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (pat, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (pat, 1));
      break;

    case 581:  /* *setcc_si_1_and */
    case 58:  /* *movdi_or */
    case 57:  /* *movsi_or */
    case 56:  /* *movdi_xor */
    case 55:  /* *movsi_xor */
    case 54:  /* *popdi1_epilogue */
    case 53:  /* *popsi1_epilogue */
    case 50:  /* *pushdi2_prologue */
    case 49:  /* *pushsi2_prologue */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (pat, 0, 0), 1));
      break;

    case 1088:  /* *movv2df_internal */
    case 1087:  /* *movv4df_internal */
    case 1086:  /* *movv4sf_internal */
    case 1085:  /* *movv8sf_internal */
    case 1084:  /* *movv1ti_internal */
    case 1083:  /* *movv2ti_internal */
    case 1082:  /* *movv2di_internal */
    case 1081:  /* *movv4di_internal */
    case 1080:  /* *movv4si_internal */
    case 1079:  /* *movv8si_internal */
    case 1078:  /* *movv8hi_internal */
    case 1077:  /* *movv16hi_internal */
    case 1076:  /* *movv16qi_internal */
    case 1075:  /* *movv32qi_internal */
    case 968:  /* *movv2sf_internal */
    case 967:  /* *movv2sf_internal_rex64 */
    case 966:  /* *movv1di_internal */
    case 965:  /* *movv2si_internal */
    case 964:  /* *movv4hi_internal */
    case 963:  /* *movv8qi_internal */
    case 962:  /* *movv1di_internal_rex64 */
    case 961:  /* *movv2si_internal_rex64 */
    case 960:  /* *movv4hi_internal_rex64 */
    case 959:  /* *movv8qi_internal_rex64 */
    case 920:  /* *prefetch_3dnow */
    case 583:  /* *setcc_qi */
    case 582:  /* *setcc_si_1_movzbl */
    case 580:  /* *setcc_di_1 */
    case 244:  /* *leadi */
    case 243:  /* *leasi */
    case 110:  /* *movsf_internal */
    case 109:  /* *movdf_internal */
    case 108:  /* *movdf_internal_rex64 */
    case 107:  /* *movxf_internal */
    case 106:  /* *movxf_internal_rex64 */
    case 105:  /* *movtf_internal_sse2 */
    case 104:  /* *movtf_internal_rex64 */
    case 103:  /* *pushsf */
    case 102:  /* *pushsf_rex64 */
    case 101:  /* *pushdf */
    case 100:  /* *pushdf_rex64 */
    case 99:  /* *pushxf_nointeger */
    case 98:  /* *pushxf */
    case 97:  /* *pushtf */
    case 66:  /* *movqi_internal */
    case 65:  /* *movhi_internal */
    case 64:  /* *movsi_internal */
    case 63:  /* *movdi_internal */
    case 62:  /* *movdi_internal_rex64 */
    case 61:  /* *movti_internal_sse */
    case 60:  /* *movti_internal_rex64 */
    case 59:  /* *movoi_internal_avx */
    case 52:  /* *popdi1 */
    case 51:  /* *popsi1 */
    case 48:  /* *pushhi2 */
    case 47:  /* *pushqi2 */
    case 46:  /* *pushsi2_rex64 */
    case 45:  /* *pushhi2_rex64 */
    case 44:  /* *pushqi2_rex64 */
    case 43:  /* *pushsi2 */
    case 42:  /* *pushdi2_rex64 */
    case 41:  /* *pushti2 */
    case 40:  /* *pushdi2 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (pat, 1));
      break;

    case 177:  /* x86_fldcw_1 */
    case 33:  /* x86_sahf_1 */
      ro[0] = *(ro_loc[0] = &XVECEXP (XEXP (pat, 1), 0, 0));
      break;

    case 1606:  /* sse_stmxcsr */
    case 950:  /* rdgsbasedi */
    case 949:  /* rdgsbasesi */
    case 948:  /* rdfsbasedi */
    case 947:  /* rdfsbasesi */
    case 942:  /* lwp_slwpcbdi */
    case 941:  /* lwp_slwpcbsi */
    case 935:  /* *rdtsc */
    case 726:  /* *load_tp_di */
    case 725:  /* *load_tp_si */
    case 724:  /* *load_tp_x32_zext */
    case 723:  /* *load_tp_x32 */
    case 647:  /* set_got_rex64 */
    case 176:  /* x86_fnstcw_1 */
    case 32:  /* x86_fnstsw_1 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      break;

    case 31:  /* *cmpfp_si_cc */
    case 30:  /* *cmpfp_hi_cc */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1));
      break;

    case 29:  /* *cmpfp_si */
    case 28:  /* *cmpfp_hi */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (XEXP (pat, 1), 0, 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XVECEXP (XEXP (pat, 1), 0, 0), 1), 0));
      ro[3] = *(ro_loc[3] = &XEXP (XVECEXP (XEXP (pat, 1), 0, 0), 1));
      break;

    case 269:  /* *addsi_4 */
    case 268:  /* *addhi_4 */
    case 267:  /* *addqi_4 */
    case 266:  /* *adddi_4 */
    case 27:  /* *cmpfp_u_cc */
    case 25:  /* *cmpfp_df_cc */
    case 24:  /* *cmpfp_sf_cc */
    case 21:  /* *cmpfp_xf_cc */
    case 19:  /* *cmpfp_0_cc */
      ro[0] = *(ro_loc[0] = &XEXP (XVECEXP (pat, 0, 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XEXP (XVECEXP (pat, 0, 0), 1), 1));
      break;

    case 26:  /* *cmpfp_u */
    case 23:  /* *cmpfp_df */
    case 22:  /* *cmpfp_sf */
    case 20:  /* *cmpfp_xf */
    case 18:  /* *cmpfp_0 */
      ro[0] = *(ro_loc[0] = &XEXP (pat, 0));
      ro[1] = *(ro_loc[1] = &XEXP (XVECEXP (XEXP (pat, 1), 0, 0), 0));
      ro[2] = *(ro_loc[2] = &XEXP (XVECEXP (XEXP (pat, 1), 0, 0), 1));
      break;

    case 17:  /* *cmpqi_ext_4 */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 0), 0));
      break;

    case 16:  /* *cmpqi_ext_3_insn_rex64 */
    case 15:  /* *cmpqi_ext_3_insn */
    case 14:  /* *cmpqi_ext_2 */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (XEXP (XEXP (pat, 1), 0), 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (pat, 1), 1));
      break;

    case 13:  /* *cmpqi_ext_1_rex64 */
    case 12:  /* *cmpqi_ext_1 */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (pat, 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (XEXP (pat, 1), 1), 0), 0));
      break;

    case 360:  /* *testsi_1 */
    case 359:  /* *testhi_1 */
    case 358:  /* *testqi_1 */
    case 357:  /* *testqi_1_maybe_si */
    case 356:  /* *testdi_1 */
    case 11:  /* *cmpdi_minus_1 */
    case 10:  /* *cmpsi_minus_1 */
    case 9:  /* *cmphi_minus_1 */
    case 8:  /* *cmpqi_minus_1 */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (XEXP (pat, 1), 0), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (XEXP (pat, 1), 0), 1));
      break;

    case 39:  /* *cmpfp_iu_387 */
    case 38:  /* *cmpfp_iu_sse */
    case 37:  /* *cmpfp_iu_mixed */
    case 36:  /* *cmpfp_i_i387 */
    case 35:  /* *cmpfp_i_sse */
    case 34:  /* *cmpfp_i_mixed */
    case 7:  /* *cmpdi_1 */
    case 6:  /* *cmpsi_1 */
    case 5:  /* *cmphi_1 */
    case 4:  /* *cmpqi_1 */
    case 3:  /* *cmpdi_ccno_1 */
    case 2:  /* *cmpsi_ccno_1 */
    case 1:  /* *cmphi_ccno_1 */
    case 0:  /* *cmpqi_ccno_1 */
      ro[0] = *(ro_loc[0] = &XEXP (XEXP (pat, 1), 0));
      ro[1] = *(ro_loc[1] = &XEXP (XEXP (pat, 1), 1));
      break;

    }
}
