/* Amalgamated selfhost hello — one TU so the in-OS tcc command line stays short
   (argv / getparameters has been flaky with long multi-file command lines). */
static unsigned long sc(long n, long a, long b, long c)
{
   unsigned long r;
   __asm__ __volatile__("int $0x30"
                        : "=a"(r)
                        : "0"(n), "b"(a), "c"(b), "d"(c)
                        : "memory");
   return r;
}

static int putchar(int c)
{
   sc(6, (long)c, 0, 0);
   return c;
}

static int puts(const char *s)
{
   while (*s)
      putchar(*s++);
   putchar('\n');
   return 0;
}

static void exit_proc(int status)
{
   sc(3, (long)status, 0, 0);
   for (;;)
      ;
}

static int main_hello(void)
{
   /* Stack string — avoid TCC's default 0x600000 .data LOAD. */
   char msg[] = {'H','e','l','l','o',' ','f','r','o','m',' ',
                 'I','C','S','-','O','S',' ','T','i','n','y','C','C',0};
   puts(msg);
   return 0;
}

void _start(void)
{
   main_hello();
   exit_proc(0);
}
