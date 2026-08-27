/* Kernel tty: canonical line discipline + VGA/serial backends. */

static tty_t tty_pool[TTY_MAX];
static int tty_used;
static tty_t *tty_foreground;
static tty_t *tty_com1;

extern void taskswitch(void);
extern void Dex32PutC(DEX32_DDL_INFO *dev, char c);
extern void serial_putc(char c);
extern unsigned char inportb(unsigned int port);
extern int get_processlist(PCB386 **buf);
extern int console_execute(const char *str);

static int tty_in_put(tty_t *t, unsigned char c)
{
   int n = (t->in_head + 1) % TTY_IBUF;
   if (n == t->in_tail)
      return -1;
   t->in_buf[t->in_head] = c;
   t->in_head = n;
   return 0;
}

static int tty_in_get(tty_t *t)
{
   unsigned char c;
   if (t->in_head == t->in_tail)
      return -1;
   c = t->in_buf[t->in_tail];
   t->in_tail = (t->in_tail + 1) % TTY_IBUF;
   return (int)c;
}

static void tty_echo(tty_t *t, char c)
{
   if (t->flags & TTY_SERIAL)
      serial_putc(c);
   if (t->ddl)
      Dex32PutC((DEX32_DDL_INFO *)t->ddl, c);
}

void tty_init(void)
{
   memset(tty_pool, 0, sizeof(tty_pool));
   tty_used = 0;
   tty_foreground = 0;
   tty_com1 = 0;
   tty_serial();
}

tty_t *tty_alloc(struct _dex32_direct_device_hdl *ddl, int flags)
{
   tty_t *t;
   if (tty_used >= TTY_MAX)
      return 0;
   t = &tty_pool[tty_used];
   memset(t, 0, sizeof(*t));
   t->id = tty_used;
   t->ddl = ddl;
   t->flags = flags ? flags : (TTY_ECHO | TTY_ICANON | TTY_ISIG);
   tty_used++;
   if (!tty_foreground && !(t->flags & TTY_SERIAL))
      tty_foreground = t;
   if ((t->flags & TTY_SERIAL) && !tty_com1)
      tty_com1 = t;
   return t;
}

tty_t *tty_get(int id)
{
   if (id < 0 || id >= tty_used)
      return 0;
   return &tty_pool[id];
}

tty_t *tty_fg(void)
{
   return tty_foreground;
}

void tty_set_fg(tty_t *t)
{
   if (t)
      tty_foreground = t;
}

tty_t *tty_serial(void)
{
   if (!tty_com1)
      tty_com1 = tty_alloc(0, TTY_ECHO | TTY_ICANON | TTY_ISIG | TTY_SERIAL);
   return tty_com1;
}

void tty_attach_proc(PCB386 *p, tty_t *t)
{
   int i;
   if (!p || !t)
      return;
   p->ctty = t;
   if (!t->session)
      t->session = (int)p->processid;
   if (!t->pgrp)
      t->pgrp = (int)p->processid;
   p->session = t->session;
   p->pgrp = t->pgrp;
   for (i = 0; i < 3; i++) {
      p->fds[i].type = FD_TTY;
      p->fds[i].ptr = t;
   }
}

static int serial_getc_poll(void)
{
   if ((inportb(0x3F8 + 5) & 1) == 0)
      return -1;
   return (int)inportb(0x3F8);
}

void tty_signal_int(tty_t *t)
{
   PCB386 *list = 0;
   int n, i;
   if (!t)
      return;
   if (current_process && (t->pgrp == 0 || current_process->pgrp == t->pgrp))
      current_process->pending_sig = SIGINT;
   n = get_processlist(&list);
   for (i = 0; i < n; i++) {
      PCB386 *p = ps_findprocess(list[i].processid);
      if (p && p != (PCB386 *)-1 && t->pgrp && p->pgrp == t->pgrp)
         p->pending_sig = SIGINT;
   }
   if (list)
      free(list);
}

void tty_input(tty_t *t, int c)
{
   if (!t || c < 0)
      return;
   if ((t->flags & TTY_ISIG) && (c == 3 || c == ('c' - 'a'))) {
      tty_signal_int(t);
      return;
   }
   if (t->flags & TTY_ICANON) {
      if (c == '\b' || c == 127 || (unsigned char)c == 145) {
         if (t->canon_len > 0) {
            t->canon_len--;
            if (t->flags & TTY_ECHO) {
               tty_echo(t, '\b');
               tty_echo(t, ' ');
               tty_echo(t, '\b');
            }
         }
         return;
      }
      if (c == '\r' || c == '\n') {
         if (t->canon_len < TTY_CANON_MAX - 1)
            t->canon[t->canon_len++] = '\n';
         t->canon[t->canon_len] = 0;
         t->line_ready = 1;
         if (t->flags & TTY_ECHO)
            tty_echo(t, '\n');
         return;
      }
      if (t->canon_len < TTY_CANON_MAX - 1) {
         t->canon[t->canon_len++] = (char)c;
         if (t->flags & TTY_ECHO)
            tty_echo(t, (char)c);
      }
      return;
   }
   tty_in_put(t, (unsigned char)c);
   if (t->flags & TTY_ECHO)
      tty_echo(t, (char)c);
}

void tty_input_fg(int c)
{
   if (tty_foreground)
      tty_input(tty_foreground, c);
}

int tty_read(tty_t *t, char *buf, int n)
{
   int i = 0;
   int c;
   if (!t || !buf || n <= 0)
      return 0;
   if (current_process && current_process->pending_sig == SIGINT) {
      current_process->pending_sig = 0;
      return -1;
   }
   if (t->flags & TTY_ICANON) {
      while (!t->line_ready) {
         if (t->flags & TTY_SERIAL) {
            c = serial_getc_poll();
            if (c >= 0)
               tty_input(t, c);
         }
         if (current_process && current_process->pending_sig == SIGINT) {
            current_process->pending_sig = 0;
            return -1;
         }
         taskswitch();
      }
      while (i < n && i < t->canon_len) {
         buf[i] = t->canon[i];
         i++;
      }
      t->canon_len = 0;
      t->line_ready = 0;
      return i;
   }
   while (i < n) {
      c = tty_in_get(t);
      if (c < 0) {
         if (t->flags & TTY_SERIAL) {
            c = serial_getc_poll();
            if (c >= 0) {
               buf[i++] = (char)c;
               continue;
            }
         }
         if (i)
            break;
         taskswitch();
         continue;
      }
      buf[i++] = (char)c;
   }
   return i;
}

int tty_write(tty_t *t, const char *buf, int n)
{
   int i;
   if (!t || !buf || n <= 0)
      return 0;
   for (i = 0; i < n; i++)
      tty_echo(t, buf[i]);
   return n;
}

int sys_kcmd(char *cmd)
{
   if (!cmd)
      return -1;
   return console_execute(cmd);
}
