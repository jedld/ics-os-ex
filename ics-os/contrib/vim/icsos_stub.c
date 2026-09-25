#include "vim.h"
#include <termcap.h>

/*
 * Minimal ICS-OS termcap shim.  Vim only needs a small number of entries;
 * the important ones are "ti"/"te", which put the kernel VT into and out of
 * the alternate screen so the primary console is restored on exit.
 */
static const char *
icsos_tcap_str(const char *id)
{
    if (id == NULL)
	return NULL;
    if (vim_stricmp(id, "ti") == 0)
	return "\033[?1049h";
    if (vim_stricmp(id, "te") == 0)
	return "\033[?1049l";
    if (vim_stricmp(id, "vi") == 0)
	return "\033[?25l";
    if (vim_stricmp(id, "ve") == 0)
	return "\033[?25h";
    if (vim_stricmp(id, "cl") == 0)
	return "\033[H\033[2J";
    if (vim_stricmp(id, "ce") == 0)
	return "\033[K";
    if (vim_stricmp(id, "cd") == 0)
	return "\033[J";
    if (vim_stricmp(id, "cm") == 0)
	return "\033[%i%d;%dH";
    if (vim_stricmp(id, "AL") == 0)
	return "\033[%dL";
    if (vim_stricmp(id, "DL") == 0)
	return "\033[%dM";
    if (vim_stricmp(id, "RI") == 0)
	return "\033[%dC";
    if (vim_stricmp(id, "le") == 0)
	return "\b";
    if (vim_stricmp(id, "mb") == 0)
	return "\033[7m";
    if (vim_stricmp(id, "me") == 0)
	return "\033[0m";
    if (vim_stricmp(id, "md") == 0)
	return "\033[1m";
    if (vim_stricmp(id, "se") == 0)
	return "\033[22m";
    if (vim_stricmp(id, "so") == 0)
	return "\033[1m";
    if (vim_stricmp(id, "ue") == 0)
	return "\033[24m";
    if (vim_stricmp(id, "us") == 0)
	return "\033[4m";
    return NULL;
}

int
tgetent(
    const char   *id,
    char	    *buf)
{
    (void)id;
    if (buf != NULL)
	buf[0] = NUL;
    return 1;
}

int
tgetflag(const char *id)
{
    if (id == NULL)
	return -1;
    if (vim_stricmp(id, "am") == 0)
	return 1;
    if (vim_stricmp(id, "ms") == 0)
	return 1;
    if (vim_stricmp(id, "ut") == 0)
	return 1;
    return 0;
}

int
tgetnum(const char *id)
{
    if (id == NULL)
	return -1;
    if (vim_stricmp(id, "co") == 0)
	return 80;
    if (vim_stricmp(id, "li") == 0)
	return 25;
    if (vim_stricmp(id, "Co") == 0)
	return 8;
    return -1;
}

char *
tgetstr(
    const char   *id,
    char	    **buf)
{
    const char  *value = icsos_tcap_str(id);
    char	    *orig;
    size_t	    len;

    if (value == NULL)
	return NULL;
    if (buf == NULL || *buf == NULL)
	return (char *)value;

    orig = *buf;
    len = vim_strsize(value);
    mch_memmove(orig, value, len + 1);
    *buf = orig + len + 1;
    return orig;
}

int
tputs(
    char	    *str,
    int	    affcnt,
    int	    (*outc)(int))
{
    (void)affcnt;
    if (str != NULL)
	while (*str != NUL)
	    outc((unsigned char)*str++);
    return 0;
}

char *
tgoto(
    char	    *cm,
    int	    col,
    int	    line)
{
    static char	buf[80];
    char	    *s = buf;
    int	    x = col;
    int	    y = line;

    if (cm == NULL)
	return "OOPS";
    while (*cm != NUL)
    {
	if (*cm != '%')
	{
	    *s++ = *cm++;
	    continue;
	}
	cm++;
	switch (*cm)
	{
	case 'd':
	    s += sprintf(s, "%d", y);
	    y = x;
	    break;
	case 'i':
	    x++;
	    y++;
	    break;
	case '+':
	    *s++ = (char)(*++cm + y);
	    y = x;
	    break;
	case 'r':
	    {
		int tmp = y;

		y = x;
		x = tmp;
	    }
	    break;
	case '%':
	    *s++ = '%';
	    break;
	case NUL:
	    break;
	default:
	    return "OOPS";
	}
	if (s - buf >= 72)
	    return "OOPS";
    }
    *s = NUL;
    return buf;
}

/*
 * FEAT_TINY link stub. get_cmd_output() normally lives in misc1.c under
 * FEAT_EVAL/HAVE_LOCALE_H and is compiled out here yet still referenced by
 * backtick expansion in filepath.c. ICS-OS has no shell, so it is inert.
 */
char_u *
get_cmd_output(
    char_u    *cmd,
    char_u    *infile,
    int       flags,
    int       *ret_len)
{
    (void)cmd;
    (void)infile;
    (void)flags;
    (void)ret_len;
    return NULL;
}
