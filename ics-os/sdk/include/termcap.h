#ifndef ICSOS_TERMCAP_H
#define ICSOS_TERMCAP_H

int tgetent(const char *id, char *buf);
int tgetflag(const char *id);
int tgetnum(const char *id);
char *tgetstr(const char *id, char **buf);
int tputs(char *str, int affcnt, int (*outc)(int));
char *tgoto(char *cm, int col, int line);

#endif
