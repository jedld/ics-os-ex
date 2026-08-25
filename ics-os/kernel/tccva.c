/* TCC 0.9.27 x86_64 emits calls to __va_arg for stdarg. */
void *__va_arg(char **ap, int size, int align)
{
   long a = (long)*ap;
   if (align < 8)
      align = 8;
   a = (a + (align - 1)) & ~(long)(align - 1);
   *ap = (char *)(a + ((size + 7) & ~7));
   return (void *)a;
}
