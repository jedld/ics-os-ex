/* Control test: a small TCC program that echoes its argc/argv.
   Built in-OS by the gcc tcc.exe and run WITH command-line arguments to
   isolate "TCC-linker output + args" from the large tccnew binary. */
extern int printf(const char *fmt, ...);

int main(int argc, char **argv)
{
    int i;
    printf("ARGS: argc=%d\n", argc);
    for (i = 0; i < argc; i++)
        printf("  argv[%d]=%s\n", i, argv[i]);
    return 0;
}
