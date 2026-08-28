/* binutils' include/binary-io.h maps the symbol `fileno` to `_fileno` when
 * O_BINARY is non-zero (the ICS-OS SDK defines O_BINARY 0x0004). The SDK's
 * canonical implementation is `fileno` (sdk/posix.c). The binutils tools
 * (ar/as/ld) therefore reference `_fileno`; this tiny alias provides that
 * name without duplicating the implementation. */
#include <stdio.h>

extern int fileno (FILE *f);
int _fileno (FILE *f) { return fileno (f); }
