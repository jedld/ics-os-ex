/* Selfhost hello using tinyio (not the full SDK). */
int puts(const char *s);
int main(int argc, char **argv)
{
   (void)argc;
   (void)argv;
   puts("Hello from ICS-OS TinyCC");
   return 0;
}
