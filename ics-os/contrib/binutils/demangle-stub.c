/* ICS-OS has no C++ runtime, so libiberty's cp-demangle.c is not built.
 * bfd.c's bfd_demangle() and ld (ldlang.c / lexsup.c) reference a small set
 * of demangler symbols unconditionally (the C++ demangle is a "nice to have"
 * for symbol display, never required for correctness). This stub satisfies
 * the link and does no demangling:
 *   - cplus_demangle()            -> NULL (bfd.c)
 *   - current_demangling_style    -> global (declared extern in demangle.h)
 *   - cplus_demangle_set_style()  -> store + return the requested style
 *   - cplus_demangle_name_to_style() -> map a name, else no_demangling
 * demangle.h is self-contained (the DMGL_* style flags are #defined in it),
 * so including it gives the exact enum + declarations with no libiberty dep.
 */
#include "demangle.h"

char *cplus_demangle (const char *mangled, int options)
{
  (void) mangled;
  (void) options;
  return (char *) 0;
}

enum demangling_styles current_demangling_style = no_demangling;

enum demangling_styles
cplus_demangle_set_style (enum demangling_styles style)
{
  current_demangling_style = style;
  return style;
}

enum demangling_styles
cplus_demangle_name_to_style (const char *name)
{
  (void) name;
  return no_demangling;
}
