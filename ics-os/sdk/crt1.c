#include "dexsdk.h"

extern int main(int argc, char **argv, char **envp);

int _start(){
   char *s;
   int c=0;
   char *p[100];
   char params[4096];

   getparameters(params);

   s=strtok(params," ");

   do {
      p[c]=s;
      c++;
      s=strtok(0," ");
   } while (s!=0 && c < 99);
   p[c] = 0;

   /* SysV third argument is envp. An empty vector (not NULL): GNU make
    * walks envp[i] until a NULL entry. Passing 0 made envp[0] read
    * identity-map address 0 (CPL0) and then GPF on a non-canonical ptr. */
   {
      static char *empty_env[1] = { 0 };
      main(c, p, empty_env);
   }
   exit(0);
   return 0;
}
