/*
 * VT100/xterm subset interpreter for DDL-backed ttys.
 *
 * Feeds user-space console output through a small escape-sequence parser so
 * full-screen applications (vim, less, top) can position the cursor, edit
 * lines, change colors, and use the alternate screen buffer on the 80x25
 * VGA text console. Serial ttys pass bytes through raw (the remote side is
 * itself a terminal).
 *
 * Supported: C0 controls, ESC [ CSI (CUP CHA VPA CUU CUD CUF CUB CNL CPL
 * ED EL ICH DCH ECH IL DL SU SD SGR DECSTBM DSR-6, DECSC/DECRST, private
 * modes ?25 ?47 ?1047 ?1048 ?1049), ESC ] OSC (ignored), ESC 7/8,
 * ESC M (RI), ESC c (RIS).
 */

#include "../dextypes.h"
#include "tty.h"
#include "dex_DDL.h"

extern void Dex32UpdateCursor(DEX32_DDL_INFO *dev, int y, int x);
extern void serial_putc(char c);
extern void outportb(unsigned int port, unsigned char value);
extern void *memset(void *s, int c, unsigned int n);
extern void *memcpy(void *d, const void *s, unsigned int n);
extern void *memmove(void *d, const void *s, unsigned int n);
extern void *malloc(unsigned int n);
extern void free(void *p);

enum { VT_S_GROUND = 0, VT_S_ESC, VT_S_CSI, VT_S_OSC };

#define VT_SCREEN_SIZE (VT_COLS * VT_ROWS * 2)

static unsigned char *vt_screen(tty_t *t)
{
   return (unsigned char *)t->ddl->buf_ptr;
}

static void vt_putcell(tty_t *t, int x, int y, char c, int attr)
{
   unsigned char *s = vt_screen(t);
   if (x < 0 || x >= VT_COLS || y < 0 || y >= VT_ROWS)
      return;
   s[(y * VT_COLS + x) * 2] = (unsigned char)c;
   s[(y * VT_COLS + x) * 2 + 1] = (unsigned char)attr;
}

static void vt_clear_rect(tty_t *t, int x1, int y1, int x2, int y2, int attr)
{
   int x, y;
   if (x1 < 0) x1 = 0;
   if (y1 < 0) y1 = 0;
   if (x2 >= VT_COLS) x2 = VT_COLS - 1;
   if (y2 >= VT_ROWS) y2 = VT_ROWS - 1;
   if (x1 > x2 || y1 > y2)
      return;
   for (y = y1; y <= y2; y++)
      for (x = x1; x <= x2; x++)
         vt_putcell(t, x, y, ' ', attr);
}

/* Scroll region [top..bot] up by one row; new bottom row is blanked. */
static void vt_scroll_up(tty_t *t, int top, int bot, int attr)
{
   unsigned char *s = vt_screen(t);
   int rows = bot - top + 1;
   if (top < 0) top = 0;
   if (bot >= VT_ROWS) bot = VT_ROWS - 1;
   if (top > bot)
      return;
   memmove(s + top * VT_COLS * 2,
           s + (top + 1) * VT_COLS * 2,
           (rows - 1) * VT_COLS * 2);
   vt_clear_rect(t, 0, bot, VT_COLS - 1, bot, attr);
}

static void vt_scroll_down(tty_t *t, int top, int bot, int attr)
{
   unsigned char *s = vt_screen(t);
   int rows = bot - top + 1;
   if (top < 0) top = 0;
   if (bot >= VT_ROWS) bot = VT_ROWS - 1;
   if (top > bot)
      return;
   memmove(s + (top + 1) * VT_COLS * 2,
           s + top * VT_COLS * 2,
           (rows - 1) * VT_COLS * 2);
   vt_clear_rect(t, 0, top, VT_COLS - 1, top, attr);
}

static void vt_cursor_clamp(tty_t *t)
{
   if (t->ddl->curx < 0) t->ddl->curx = 0;
   if (t->ddl->curx >= VT_COLS) t->ddl->curx = VT_COLS - 1;
   if (t->ddl->cury < 0) t->ddl->cury = 0;
   if (t->ddl->cury >= VT_ROWS) t->ddl->cury = VT_ROWS - 1;
}

static void vt_cursor_sync(tty_t *t)
{
   vt_cursor_clamp(t);
   Dex32UpdateCursor((DEX32_DDL_INFO *)t->ddl, t->ddl->cury, t->ddl->curx);
}

/* Move down one line honoring the scroll region (VT "index" behavior). */
static void vt_index(tty_t *t, int attr)
{
   vt_state_t *v = &t->vt;
   if (t->ddl->cury == v->stb_bot)
      vt_scroll_up(t, v->stb_top, v->stb_bot, attr);
   else if (t->ddl->cury < VT_ROWS - 1)
      t->ddl->cury++;
   vt_cursor_sync(t);
}

static void vt_reverse_index(tty_t *t, int attr)
{
   vt_state_t *v = &t->vt;
   if (t->ddl->cury == v->stb_top)
      vt_scroll_down(t, v->stb_top, v->stb_bot, attr);
   else if (t->ddl->cury > 0)
      t->ddl->cury--;
   vt_cursor_sync(t);
}

static void vt_putchar(tty_t *t, char c, int attr)
{
   int x = t->ddl->curx;
   if (x >= VT_COLS) {
      x = 0;
      vt_index(t, attr);
   }
   vt_putcell(t, x, t->ddl->cury, c, attr);
   t->ddl->curx = x + 1;
   if (t->ddl->curx >= VT_COLS)
      t->ddl->curx = 0;
   vt_cursor_sync(t);
}

/* Parse "n1;n2;..." into vals; returns count. -1 on malformed input. */
static int vt_parse_params(char *s, int *vals, int max)
{
   int n = 0, cur = -1, i;
   for (i = 0; s[i] && n < max; i++) {
      if (s[i] >= '0' && s[i] <= '9') {
         if (cur < 0)
            cur = 0;
         cur = cur * 10 + (s[i] - '0');
      } else if (s[i] == ';') {
         vals[n++] = cur < 0 ? 0 : cur;
         cur = -1;
      } else {
         return -1;
      }
   }
   vals[n++] = cur < 0 ? 0 : cur;
   return n;
}

static void vt_apply_sgr(tty_t *t, char *params)
{
   vt_state_t *v = &t->vt;
   int vals[16], n, i;
   n = vt_parse_params(params, vals, 16);
   if (n <= 0)
      return;
   for (i = 0; i < n; i++) {
      int p = vals[i];
      switch (p) {
      case 0:
         v->sgr = 0x07;
         break;
      case 1:
         v->sgr = (v->sgr & 0xF0) | 0x0F;
         break;
      case 4:
         v->sgr |= 0x80;
         break;
      case 7: {
         int fg = v->sgr & 0x0F, bg = (v->sgr >> 4) & 0x0F;
         v->sgr = (v->sgr & 0x80) | (fg << 4) | bg;
         break;
      }
      case 21: case 22:
         v->sgr = (v->sgr & 0xF0) | (v->sgr & 0x07);
         break;
      case 24:
         v->sgr &= ~0x80;
         break;
      case 27: {
         int fg = v->sgr & 0x0F, bg = (v->sgr >> 4) & 0x0F;
         v->sgr = (v->sgr & 0x80) | fg | (bg << 4);
         break;
      }
      case 39:
         v->sgr = (v->sgr & 0xF0) | 0x07;
         break;
      case 49:
         v->sgr &= 0x0F;
         break;
      default:
         if (p >= 30 && p <= 37)
            v->sgr = (v->sgr & 0xF0) | (p - 30);
         else if (p >= 40 && p <= 47)
            v->sgr = (v->sgr & 0x0F) | ((p - 40) << 4);
         else if (p >= 90 && p <= 97)
            v->sgr = (v->sgr & 0xF0) | (p - 90 + 8);
         else if (p >= 100 && p <= 107)
            v->sgr = (v->sgr & 0x0F) | ((p - 100 + 8) << 4);
         break;
      }
   }
}

/* Format a non-negative integer into dst (no leading zeros); return len. */
static int vt_fmt_num(char *dst, int val)
{
   char tmp[12];
   int n = 0, i = 0;
   if (val == 0) { dst[0] = '0'; return 1; }
   while (val > 0) { tmp[n++] = (char)('0' + (val % 10)); val /= 10; }
   while (n > 0) dst[i++] = tmp[--n];
   return i;
}

 /* Respond to DSR-6 by injecting CSI row;col R into the tty input queue. */
static void vt_dsr_response(tty_t *t)
{
    char buf[24];
    int row = t->ddl->cury + 1, col = t->ddl->curx + 1, i = 0, j;
    buf[i++] = 0x1B; buf[i++] = '[';
    i += vt_fmt_num(buf + i, row);
    buf[i++] = ';';
    i += vt_fmt_num(buf + i, col);
    buf[i++] = 'R';
    for (j = 0; j < i; j++)
       tty_inject(t, buf[j]);
 }

static void vt_ris(tty_t *t)
{
   vt_state_t *v = &t->vt;
   v->state = VT_S_GROUND;
   v->sgr = 0x07;
   v->stb_top = 0;
   v->stb_bot = VT_ROWS - 1;
   v->savx = v->savy = 0;
   v->savsgr = 0x07;
   vt_screen_clear(v, t->ddl, 0x07);
   t->ddl->curx = 0;
   t->ddl->cury = 0;
   t->ddl->attb = 0x07;
   vt_cursor_sync(t);
}

static void vt_alt_enter(tty_t *t, int savecursor, int clear)
{
   vt_state_t *v = &t->vt;
   if (v->alt)
      return;
   if (!v->altbuf) {
      v->altbuf = (unsigned char *)malloc(VT_SCREEN_SIZE);
      if (!v->altbuf)
         return;
   }
   if (savecursor) {
      v->savx = t->ddl->curx;
      v->savy = t->ddl->cury;
      v->savsgr = v->sgr;
   }
   memcpy(v->altbuf, vt_screen(t), VT_SCREEN_SIZE);
   if (clear)
      vt_screen_clear(v, t->ddl, v->sgr);
   v->alt = 1;
}

static void vt_alt_exit(tty_t *t, int restorecursor)
{
   vt_state_t *v = &t->vt;
   if (!v->alt || !v->altbuf)
      return;
   v->alt = 0;
   memcpy(vt_screen(t), v->altbuf, VT_SCREEN_SIZE);
   if (restorecursor) {
      t->ddl->curx = v->savx;
      t->ddl->cury = v->savy;
      v->sgr = v->savsgr;
   }
   vt_cursor_sync(t);
}

static void vt_csi_dispatch(tty_t *t, char final)
{
   vt_state_t *v = &t->vt;
   char params[24];
   int n = v->csi_n;
   int vals[16];
   int cnt, p0, p1, private;

   private = (n > 0 && v->csi[0] == '?') ? 1 : 0;
   if (private) {
      n--;
      if (n > 0)
         memcpy(params, v->csi + 1, n);
   } else if (n > 0) {
      memcpy(params, v->csi, n);
   }
   params[n < (int)sizeof(params) ? n : (int)sizeof(params) - 1] = 0;

   cnt = vt_parse_params(params, vals, 16);
   if (cnt < 0)
      cnt = 0;
   p0 = cnt > 0 ? vals[0] : 0;
   p1 = cnt > 1 ? vals[1] : 0;

   switch (final) {
   case 'A':
      if (p0 <= 0) p0 = 1;
      t->ddl->cury -= p0;
      vt_cursor_sync(t);
      break;
   case 'B': case 'e':
      if (p0 <= 0) p0 = 1;
      t->ddl->cury += p0;
      vt_cursor_sync(t);
      break;
   case 'C':
      if (p0 <= 0) p0 = 1;
      t->ddl->curx += p0;
      vt_cursor_sync(t);
      break;
   case 'D':
      if (p0 <= 0) p0 = 1;
      t->ddl->curx -= p0;
      vt_cursor_sync(t);
      break;
   case 'E':
      if (p0 <= 0) p0 = 1;
      t->ddl->cury += p0;
      t->ddl->curx = 0;
      vt_cursor_sync(t);
      break;
   case 'F':
      if (p0 <= 0) p0 = 1;
      t->ddl->cury -= p0;
      t->ddl->curx = 0;
      vt_cursor_sync(t);
      break;
   case 'G': case '`':
      t->ddl->curx = (p0 <= 0 ? 1 : p0) - 1;
      vt_cursor_sync(t);
      break;
   case 'd':
      t->ddl->cury = (p0 <= 0 ? 1 : p0) - 1;
      vt_cursor_sync(t);
      break;
   case 'H': case 'f':
      t->ddl->cury = (p0 <= 0 ? 1 : p0) - 1;
      t->ddl->curx = (p1 <= 0 ? 1 : p1) - 1;
      vt_cursor_sync(t);
      break;
   case 'J':
      switch (p0) {
      case 0:
         vt_clear_rect(t, t->ddl->curx, t->ddl->cury, VT_COLS - 1, t->ddl->cury, v->sgr);
         vt_clear_rect(t, 0, t->ddl->cury + 1, VT_COLS - 1, VT_ROWS - 1, v->sgr);
         break;
      case 1:
         vt_clear_rect(t, 0, 0, VT_COLS - 1, t->ddl->cury - 1, v->sgr);
         vt_clear_rect(t, 0, t->ddl->cury, t->ddl->curx, t->ddl->cury, v->sgr);
         break;
      default:
         vt_screen_clear(v, t->ddl, v->sgr);
         break;
      }
      break;
   case 'K':
      switch (p0) {
      case 0:
         vt_clear_rect(t, t->ddl->curx, t->ddl->cury, VT_COLS - 1, t->ddl->cury, v->sgr);
         break;
      case 1:
         vt_clear_rect(t, 0, t->ddl->cury, t->ddl->curx, t->ddl->cury, v->sgr);
         break;
      default:
         vt_clear_rect(t, 0, t->ddl->cury, VT_COLS - 1, t->ddl->cury, v->sgr);
         break;
      }
      break;
   case '@': {
      int x = t->ddl->curx, y = t->ddl->cury;
      unsigned char *s = vt_screen(t);
      int count = p0 <= 0 ? 1 : p0;
      unsigned char *dst = s + (y * VT_COLS + x) * 2;
      if (count > VT_COLS - x)
         count = VT_COLS - x;
      memmove(dst + count * 2, dst, (VT_COLS - x - count) * 2);
      vt_clear_rect(t, x, y, x + count - 1, y, v->sgr);
      break;
   }
   case 'P': {
      int x = t->ddl->curx, y = t->ddl->cury;
      unsigned char *s = vt_screen(t);
      int count = p0 <= 0 ? 1 : p0;
      unsigned char *dst = s + (y * VT_COLS + x) * 2;
      if (count > VT_COLS - x)
         count = VT_COLS - x;
      memmove(dst, dst + count * 2, (VT_COLS - x - count) * 2);
      vt_clear_rect(t, VT_COLS - count, y, VT_COLS - 1, y, v->sgr);
      break;
   }
   case 'X': {
      int count = p0 <= 0 ? 1 : p0;
      vt_clear_rect(t, t->ddl->curx, t->ddl->cury,
                    t->ddl->curx + count - 1, t->ddl->cury, v->sgr);
      break;
   }
   case 'L': {
      int y = t->ddl->cury, count = p0 <= 0 ? 1 : p0;
      unsigned char *s = vt_screen(t);
      if (y < v->stb_top || y > v->stb_bot)
         break;
      if (count > v->stb_bot - y + 1)
         count = v->stb_bot - y + 1;
      memmove(s + (y + count) * VT_COLS * 2,
              s + y * VT_COLS * 2,
              (v->stb_bot - y + 1 - count) * VT_COLS * 2);
      vt_clear_rect(t, 0, y, VT_COLS - 1, y + count - 1, v->sgr);
      break;
   }
   case 'M': {
      int y = t->ddl->cury, count = p0 <= 0 ? 1 : p0;
      unsigned char *s = vt_screen(t);
      if (y < v->stb_top || y > v->stb_bot)
         break;
      if (count > v->stb_bot - y + 1)
         count = v->stb_bot - y + 1;
      memmove(s + y * VT_COLS * 2,
              s + (y + count) * VT_COLS * 2,
              (v->stb_bot - y + 1 - count) * VT_COLS * 2);
      vt_clear_rect(t, 0, v->stb_bot - count + 1, VT_COLS - 1, v->stb_bot, v->sgr);
      break;
   }
   case 'S':
      if (p0 <= 0) p0 = 1;
      while (p0-- > 0)
         vt_scroll_up(t, v->stb_top, v->stb_bot, v->sgr);
      break;
   case 'T':
      if (p0 <= 0) p0 = 1;
      while (p0-- > 0)
         vt_scroll_down(t, v->stb_top, v->stb_bot, v->sgr);
      break;
   case 'm':
      vt_apply_sgr(t, params);
      break;
   case 'n':
      if (p0 == 6)
         vt_dsr_response(t);
      break;
   case 'r':
      if (p0 <= 0) p0 = 1;
      if (p1 <= 0) p1 = VT_ROWS;
      if (p0 < 1 || p1 > VT_ROWS || p0 >= p1)
         break;
      v->stb_top = p0 - 1;
      v->stb_bot = p1 - 1;
      t->ddl->curx = 0;
      t->ddl->cury = 0;
      vt_cursor_sync(t);
      break;
   case 's':
      v->savx = t->ddl->curx;
      v->savy = t->ddl->cury;
      break;
   case 'u':
      t->ddl->curx = v->savx;
      t->ddl->cury = v->savy;
      vt_cursor_sync(t);
      break;
   case 'h': case 'l':
      if (!private)
         break;
      switch (p0) {
      case 25:
         vt_cursor_set_visible((DEX32_DDL_INFO *)t->ddl, final == 'h');
         v->curhidden = (final == 'l');
         break;
      case 47:
         if (final == 'h')
            vt_alt_enter(t, 0, 1);
         else
            vt_alt_exit(t, 0);
         break;
      case 1047:
         if (final == 'h')
            vt_alt_enter(t, 0, 0);
         else
            vt_alt_exit(t, 0);
         break;
      case 1048:
         if (final == 'h') {
            v->savx = t->ddl->curx;
            v->savy = t->ddl->cury;
         } else {
            t->ddl->curx = v->savx;
            t->ddl->cury = v->savy;
            vt_cursor_sync(t);
         }
         break;
      case 1049:
         if (final == 'h')
            vt_alt_enter(t, 1, 1);
         else
            vt_alt_exit(t, 1);
         break;
      default:
         break;
      }
      break;
   default:
      break;
   }
}

void vt_init(vt_state_t *v)
{
   memset(v, 0, sizeof(*v));
   v->state = VT_S_GROUND;
   v->sgr = 0x07;
   v->stb_top = 0;
   v->stb_bot = VT_ROWS - 1;
   v->altbuf = 0;
}

void vt_screen_clear(vt_state_t *v, struct _dex32_direct_device_hdl *ddl, int attr)
{
   int i;
   (void)v;
   memset(ddl->buf_ptr, ' ', VT_SCREEN_SIZE);
   for (i = 1; i < VT_SCREEN_SIZE; i += 2)
      ((unsigned char *)ddl->buf_ptr)[i] = (unsigned char)attr;
}

void vt_cursor_set_visible(struct _dex32_direct_device_hdl *ddl, int visible)
{
   (void)ddl;
   /* VGA CRTC cursor scan lines: index 0x0A = start, 0x0C = end.
      Setting the start to 0x18 (out of range) hides the hardware
      cursor; 0x0C..0x0E restores the normal half-height cursor. */
   if (!visible) {
      outportb(0x3D4, 0x0A);
      outportb(0x3D5, 0x18);
      outportb(0x3D4, 0x0C);
      outportb(0x3D5, 0x18);
   } else {
      outportb(0x3D4, 0x0A);
      outportb(0x3D5, 0x0C);
      outportb(0x3D4, 0x0C);
      outportb(0x3D5, 0x0E);
   }
}

void vt_feed(tty_t *t, int c)
{
   vt_state_t *v;
   if (!t)
      return;
   if (t->flags & TTY_SERIAL) {
      serial_putc((char)c);
      return;
   }
   if (!t->ddl)
      return;
   v = &t->vt;

   switch (v->state) {
   case VT_S_OSC:
      if (c == 0x07)
         v->state = VT_S_GROUND;
      else if (c == 0x1B)
         v->state = VT_S_ESC;
      return;

   case VT_S_ESC:
      if (c == '[') {
         v->state = VT_S_CSI;
         v->csi_n = 0;
      } else if (c == ']') {
         v->state = VT_S_OSC;
      } else if (c == '7') {
         v->savx = t->ddl->curx;
         v->savy = t->ddl->cury;
         v->savsgr = v->sgr;
         v->state = VT_S_GROUND;
      } else if (c == '8') {
         t->ddl->curx = v->savx;
         t->ddl->cury = v->savy;
         v->sgr = v->savsgr;
         v->state = VT_S_GROUND;
         vt_cursor_sync(t);
      } else if (c == 'M') {
         vt_reverse_index(t, v->sgr);
         v->state = VT_S_GROUND;
      } else if (c == 'c') {
         vt_ris(t);
      } else if (c != 0x1B) {
         v->state = VT_S_GROUND;
      }
      return;

   case VT_S_CSI:
      if (c == 0x1B) {
         v->state = VT_S_ESC;
         return;
      }
      if (c >= 0x40 && c <= 0x7E) {
         vt_csi_dispatch(t, (char)c);
         v->state = VT_S_GROUND;
         return;
      }
      if (c >= 0x20 && c <= 0x3F) {
          /* 0x20-0x2F intermediate bytes, 0x30-0x3F parameter bytes
             (digits, ';', ':', and the '?' private marker). */
          if (v->csi_n < (int)sizeof(v->csi) - 1)
             v->csi[v->csi_n++] = (char)c;
       } else {
          /* control char or overflow: abort sequence */
          v->state = VT_S_GROUND;
       }
       return;

   default:
      break;
   }

   /* ground state */
   switch (c) {
   case 0x1B:
      v->state = VT_S_ESC;
      return;
   case '\n':
      vt_index(t, v->sgr);
      return;
   case '\r':
      t->ddl->curx = 0;
      vt_cursor_sync(t);
      return;
   case '\b':
      if (t->ddl->curx > 0)
         t->ddl->curx--;
      vt_cursor_sync(t);
      return;
   case '\t':
      while (t->ddl->curx < VT_COLS && (t->ddl->curx & 7) != 0)
         t->ddl->curx++;
      vt_cursor_sync(t);
      return;
   case '\a':
      return;
   default:
      if (c < 0x20)
         return;
      vt_putchar(t, (char)c, v->sgr);
      return;
   }
}
