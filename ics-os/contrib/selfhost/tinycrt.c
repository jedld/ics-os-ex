/* Minimal CRT for selfhost hello — no .data argv strings. */
extern int main(void);
void exit(int status);

void _start(void)
{
   main();
   exit(0);
}
