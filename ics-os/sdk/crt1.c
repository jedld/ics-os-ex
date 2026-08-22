#include "dexsdk.h"

extern int main(int argc, char **argv);

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

   main(c,p);
   exit(0);
   return 0;
}
