#ifndef _SDK_WCHAR_H
#define _SDK_WCHAR_H

/* Minimal freestanding <wchar.h> for the ICS-OS SDK.
   GAS (gas/read.c) includes <wchar.h> but uses no wide-character routines
   (ASCII-only assembler source). Only the fundamental types and limits are
   provided; wide-char conversion/IO functions are not implemented. */

#include <stddef.h>
#include <string.h>

typedef int wint_t;
typedef int mbstate_t;

typedef struct {
    unsigned long long value;
    int count;
} fpos_t;

#ifndef WCHAR_MIN
#define WCHAR_MIN 0
#endif
#ifndef WCHAR_MAX
#define WCHAR_MAX 0x7fffffff
#endif

#define WEOF ((wint_t)-1)

/* The only wide-char conversion GAS uses: mbstowcs(NULL, name, len) just to
   test whether a quoted symbol name is representable in the current locale.
   ICS-OS is single-byte/ASCII, so the conversion always succeeds and the
   result is simply the byte length. */
static inline size_t mbstowcs(wchar_t *pwcs, const char *s, size_t n)
{
    size_t i = 0;
    if (!s)
        return 0;
    if (n == (size_t)-1)
        n = strlen(s);
    while (i < n && s[i]) {
        if (pwcs)
            pwcs[i] = (unsigned char)s[i];
        i++;
    }
    if (pwcs)
        pwcs[i] = 0;
    return i;
}

/* WCTYPE macros (C89/C99 basic set) */
#define ISWEOF(wc) ((wc) == WEOF)
#define ISWSPACE(wc) 0
#define ISWDIGIT(wc) 0
#define ISWALNUM(wc) 0
#define ISWALPHA(wc) 0
#define ISWPUNCT(wc) 0
#define ISWPRINT(wc) 0
#define ISWGRAPH(wc) 0
#define ISWUPPER(wc) 0
#define ISWLOWER(wc) 0
#define ISWXDIGIT(wc) 0

#endif /* _SDK_WCHAR_H */
