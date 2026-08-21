/* Tiny stdio for selfhost hello — avoids compiling the full SDK. */
static unsigned int sc(int n, int a, int b, int c, int d, int e)
{
   unsigned int r;
   __asm__ __volatile__("int $0x30"
                        : "=a"(r)
                        : "0"(n), "b"(a), "c"(b), "d"(c), "S"(d), "D"(e));
   return r;
}

int putchar(int c)
{
   sc(0x6, c, 0, 0, 0, 0); /* FXN_DPUTC */
   return c;
}

int puts(const char *s)
{
   while (*s)
      putchar(*s++);
   putchar('\n');
   return 0;
}

int printf(const char *fmt, ...)
{
   /* Only supports plain strings for the smoke test. */
   const char *p = fmt;
   while (*p) {
      if (*p == '%' && p[1]) {
         p += 2;
         continue;
      }
      putchar(*p++);
   }
   return 0;
}

void exit(int status)
{
   sc(3, status, 0, 0, 0, 0);
   for (;;)
      ;
}
