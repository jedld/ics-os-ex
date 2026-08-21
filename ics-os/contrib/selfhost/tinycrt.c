/* Minimal CRT for selfhost hello — no getparameters/strtok dependency. */
extern int main(int argc, char **argv);
void exit(int status);

int _start(void)
{
   static char *argv[] = { "hello", 0 };
   main(1, argv);
   exit(0);
   return 0;
}
