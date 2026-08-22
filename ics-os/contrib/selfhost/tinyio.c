/* Tiny stdio for selfhost hello — avoids compiling the full SDK.
   Keep this free of varargs/SSE (TCC va_list can emit movq %xmm*). */
static unsigned long sc(long n, long a, long b, long c)
{
   unsigned long r;
   __asm__ __volatile__("int $0x30"
                        : "=a"(r)
                        : "0"(n), "b"(a), "c"(b), "d"(c)
                        : "memory");
   return r;
}

int putchar(int c)
{
   sc(6, (long)c, 0, 0); /* FXN_DPUTC */
   return c;
}

int puts(const char *s)
{
   while (*s)
      putchar(*s++);
   putchar('\n');
   return 0;
}

void exit(int status)
{
   sc(3, (long)status, 0, 0);
   for (;;)
      ;
}
