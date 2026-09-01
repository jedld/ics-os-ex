/*
  Name: sync.c
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 18/01/04 06:27
  Description: Provides process synchornization functions
*/

#include "sync.h"
#include "../cpu/smp.h"

static int sync_owner_token(void)
{
   /* Recursion is valid only in the same process on the same CPU. Encoding
      cpu+1 keeps zero reserved for the unlocked state, including pid zero. */
   return (int)(((getprocessid() & 0x007FFFFF) << 8) |
                ((smp_cpu_id() + 1) & 0xFF));
}


//perform busy waiting
void sync_justwait(sync_sharedvar *var){
   int owner=sync_owner_token();
   int held;
   do {
      held=__sync_val_compare_and_swap(&var->busy,0,0);
      if (held && held!=owner)
         __asm__ __volatile__("pause");
   } while (held && held!=owner);
};

//attempt to enter the critical section
void sync_entercrit(sync_sharedvar *var){
   int owner=sync_owner_token();

   if (__sync_val_compare_and_swap(&var->busy,0,0)==owner) {
      var->wait++;
      return;
   }

   while (!__sync_bool_compare_and_swap(&var->busy,0,owner))
      __asm__ __volatile__("pause");
   var->wait=1;
};

//leave the critical section
void sync_leavecrit(sync_sharedvar *var){
   int owner=sync_owner_token();

   if (__sync_val_compare_and_swap(&var->busy,0,0)!=owner) {
      printf("sync: warning critical section released by non-owner!\n");
      return;
   }
   var->wait--;

   if (var->wait<0) 
      printf("sync: warning wrong number of enter-leave pairs detected!\n");

   if (var->wait==0)
      __sync_lock_release(&var->busy);
};

