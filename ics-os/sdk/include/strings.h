#ifndef _STRINGS_H
#define _STRINGS_H

/* POSIX <strings.h>: case-insensitive string comparison. Provided by
   libiberty strcasecmp.c / strncasecmp.c (linked into the tools). */

int strcasecmp(const char *s1, const char *s2);
int strncasecmp(const char *s1, const char *s2, unsigned int n);

#endif
