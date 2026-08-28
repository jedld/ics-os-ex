/* ICS-OS has no C++ runtime, so libiberty's cp-demangle.c is not built.
 * bfd.c's bfd_demangle() calls cplus_demangle() unconditionally; this stub
 * satisfies the link and returns NULL (no demangling). */

char *cplus_demangle (const char *mangled, int options)
{
  (void) mangled;
  (void) options;
  return (char *) 0;
}
