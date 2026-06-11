/*
threads.c - basic multithreading demo for ICS-OS.

Demonstrates the thread API:
   thread_create(f)  - start f as a new thread of this process
   thread_join(tid)  - wait for a thread to terminate
   thread_exit()     - terminate the calling thread early
A thread function may also simply return: thread_create() seeds
the stack so that returning lands in thread_exit() automatically.

Threads share the process address space, so the global counters
below are visible to everyone - which is also how data races
happen. The SDK library is not thread-safe; keep printf calls
infrequent.
*/

#include "../../sdk/dexsdk.h"

#define WORK  5          /*progress lines each worker prints*/
#define SLICE 200000   /*busy iterations between progress lines*/

volatile unsigned int count1 = 0, count2 = 0;

/*worker1 finishes its work and just returns */
void worker1(){
   int r;
   volatile unsigned int i;
   for (r = 1; r <= WORK; r++){
      for (i = 0; i < SLICE; i++) count1++;
      printf("  worker1: step %d of %d (count1=%u)\n", r, WORK, count1);
   }
}

/*worker2 bails out early with thread_exit()*/
void worker2(){
   int r;
   volatile unsigned int i;
   for (r = 1; r <= WORK; r++){
      for (i = 0; i < SLICE; i++) count2++;
      printf("  worker2: step %d of %d (count2=%u)\n", r, WORK, count2);
      if (r == 3){
         printf("  worker2: had enough, exiting early.\n");
         thread_exit();      /*terminates this thread only*/
      }
   }
}

int main(int argc, char *argv[]){
   int tid1, tid2;

   printf("threads demo: main pid = %d\n", getpid());

   tid1 = thread_create((void*)worker1);
   tid2 = thread_create((void*)worker2);
   printf("main: started worker1 (tid %d) and worker2 (tid %d)\n",
            tid1, tid2);
   printf("main: run 'ps' on another console to see the .thread entries\n");

   /*block until each worker terminates */
   thread_join(tid1);
   printf("main: worker1 joined.\n");

   thread_join(tid2);
   printf("main: worker2 joined.\n");

   /*joining something that is not our thread must fail*/
   if (thread_join(1) == -1)
      printf("main: joining a non-thread pid correctly returned -1.\n");

   /*joining an already-exited thread must succeed immediately*/
   if (thread_join(tid1) == 0)
      printf("main: re-joining a finished thread correctly returned 0.\n");

   printf("main: done. count1=%u count2=%u (expect count2 ~3/5 of count1)\n",
            count1, count2);
   return 0;
}
