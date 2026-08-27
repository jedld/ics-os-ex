#ifndef ICSOS_TTY_H
#define ICSOS_TTY_H

#define TTY_IBUF      256
#define TTY_CANON_MAX 256
#define TTY_MAX       8

#define TTY_ECHO     0x01
#define TTY_ICANON   0x02
#define TTY_ISIG     0x04
#define TTY_SERIAL    0x08

#define SIGINT       2

struct _dex32_direct_device_hdl;
struct _PCB386;

typedef struct _tty {
   int id;
   int flags;
   struct _dex32_direct_device_hdl *ddl;
   int session;
   int pgrp;
   volatile int in_head, in_tail;
   unsigned char in_buf[TTY_IBUF];
   int canon_len;
   char canon[TTY_CANON_MAX];
   volatile int line_ready;
} tty_t;

void tty_init(void);
tty_t *tty_alloc(struct _dex32_direct_device_hdl *ddl, int flags);
tty_t *tty_get(int id);
tty_t *tty_fg(void);
void tty_set_fg(tty_t *t);
void tty_attach_proc(PCB386 *p, tty_t *t);
void tty_input(tty_t *t, int c);
void tty_input_fg(int c);
int tty_read(tty_t *t, char *buf, int n);
int tty_write(tty_t *t, const char *buf, int n);
void tty_signal_int(tty_t *t);
tty_t *tty_serial(void);
int sys_kcmd(char *cmd);

#endif
