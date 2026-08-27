/*
  Name: Exception handling module
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 13/03/04 06:30
  Description: This module provides exception handlers for the operating system. There are
  exception handlers for GPFs, page faults and divide by zero. The page fault handler also
  handles demand loading requests.
  
    DEX educational extensible operating system 1.0 Beta
    Copyright (C) 2004  Joseph Emmanuel DL Dayo

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. 
*/

void GPFhandler(DWORD address)
  {
  char temp[255];
    stopints();
    
    exc_showdump(address,GENERAL_PROTECTION_FAULT,0);
    exc_recover();
        
    while (1) {};

  };

/* x86_64 #13 handler with full fault context (RIP, CR2, error code, CS/SS/RSP, CR3).
   The legacy saveregs in the PCB is not populated by the software context switch,
   so this prints live values captured in the wrapper instead. */
void GPFhandler64(struct gpf_info *fi, unsigned long saved_rax, unsigned long saved_rcx)
  {
    static volatile int gpf_busy = 0;
    unsigned int err = fi->err;
    const char *what = "unknown";

    if (gpf_busy) {
       serial_puts("GPF64: re-entered -> halt\n");
       while (1) {}
    }
    gpf_busy = 1;

    if (err & 2)
       what = (err & 1) ? "reserved-bit" : "segment-not-present";
    else if (err & 4)
       what = (err & 1) ? "TSS-invalid" : (err & 2) ? "stack" : "TSS-bad";
    else
       what = (err & 1) ? "base/limit" : "segment-index";

    {
       char line[160];
       sprintf(line,
               "GPF64: err=0x%x(%s) rip=0x%llx cr2=0x%llx proc=%s\n",
               (unsigned)err, what, fi->rip, fi->cr2,
               current_process ? current_process->name : "?");
       serial_puts(line);
    }
    serial_puts("GPF64 rax=");
    {
       int i;
       for (i = 60; i >= 0; i -= 4) {
          unsigned x = (unsigned)((saved_rax >> i) & 15);
          serial_putc((char)(x < 10 ? '0' + x : 'a' + x - 10));
       }
    }
    serial_puts(" rcx=");
    {
       int i;
       for (i = 60; i >= 0; i -= 4) {
          unsigned x = (unsigned)((saved_rcx >> i) & 15);
          serial_putc((char)(x < 10 ? '0' + x : 'a' + x - 10));
       }
    }
    serial_puts(" rsp=");
    {
       int i;
       unsigned long saved_rsp = (unsigned long)fi->rsp;
       for (i = 60; i >= 0; i -= 4) {
          unsigned x = (unsigned)((saved_rsp >> i) & 15);
          serial_putc((char)(x < 10 ? '0' + x : 'a' + x - 10));
       }
    }
    serial_puts("\nGPF64: halt\n");
    while (1) {}
  };
  
void exc_invalidtss(DWORD address)
  {
  char temp[255];
    stopints();
    
    exc_showdump(address,INVALID_TSS,0);
    exc_recover();
        
    while (1) {};
    startints();

  };

/* #8 double fault diagnostic. Called by doublefaultwrapper with the
   hardware-frame RIP/CS/RFLAGS and CR2. Prints then halts. */
void exc_doublefault(unsigned long rip, unsigned long cs,
                     unsigned long rflags, unsigned long cr2)
  {
    char line[160];
    stopints();
    sprintf(line, "DBLFLT: rip=0x%lx cs=0x%04lx rflags=0x%lx cr2=0x%lx\n",
            rip, cs & 0xFFFF, rflags, cr2);
    serial_puts(line);
    while (1) {}
  };

void divide_error(DWORD address)
  {
    stopints();
    exc_showdump(address,DIVIDE_ERROR,0);
    exc_recover();
    while (1) {};
    startints();

  };
  

//show the contents of the EFLAGS register
void exc_dumpflags(DEX32_DDL_INFO *output, DWORD flags)
{
    int ID,VIP,VIF,AC,VM,RF,NT,IOPL,OF;
    int DF,IF,TF,SF,ZF,AF,PF,CF;
    
    //Perform a bit by bit comparitson
    CF   =  (flags & 1) ?   1 : 0;
    PF   =  (flags & 4) ?   1 : 0;
    AF   =  (flags & 16) ?  1 : 0;
    ZF   =  (flags & 64) ?  1 : 0;
    SF   =  (flags & 128) ? 1 : 0;
    TF   =  (flags & 256) ? 1 : 0;
    IF   =  (flags & 512) ? 1 : 0;
    DF   =  (flags & 1024) ? 1 : 0;
    OF   =  (flags & 2048) ? 1 : 0;
    IOPL =  (flags & 0x3000) >> 12;
    NT   =  (flags & 0x4000) ? 1 : 0;
    RF   =  (flags & 0x10000) ? 1 : 0;
    VM   =  (flags & 0x20000) ? 1 : 0;
    AC   =  (flags & 0x40000) ? 1 : 0;
    
    DDLprintf(output,
    "CF=%d PF=%d AF=%d ZF=%d SF=%d TF=%d IF=%d DF=%d OF=%d IO=%d NT=%d RF=%d VM=%d AC=%d\n",
    CF,PF,AF,ZF,SF,TF,IF,DF,OF,IOPL,NT,RF,VM,AC);

};



//show the contents of the CPU registers and other information
void exc_showdump(DWORD location,int type,DWORD pf_info)
{
   int ret;
   char fault_type[80];
   DWORD pageentry,direntry,ktopheap = knext;
   DEX32_DDL_INFO *beforeout,*showdumpout;


   strcpy(fault_type,"Exception -");

   //convert the value given in type to string  
   if (location >= 0xFFFF0000 && location <=0xFFFFFFF0) strcat(fault_type,"unresolved import error");
      else
   if (type == PAGE_FAULT) sprintf(fault_type,"Page fault (0x%x)",pf_info);
      else
   if (type == GENERAL_PROTECTION_FAULT) strcat(fault_type,"General Protection fault");
      else
   if (type == DIVIDE_ERROR) strcat(fault_type,"Divide by zero ");
      else
   if (type == INVALID_TSS) strcat(fault_type,"Invalid Task State Segment");
      else
   strcat(fault_type,"unknown fault");   
   
   #ifdef FULLSCREENERROR   
   direntry=getpagetablephys(location, current_process->pagedirloc);
   pageentry=getphys(location,current_process->pagedirloc);
   showdumpout = Dex32CreateDDL();
   beforeout = Dex32SetActiveDDL(showdumpout);
   
   Dex32Clear(showdumpout);
   Dex32SetTextBackground(showdumpout,RED);
   Dex32SetTextColor(showdumpout,WHITE);
   DDLprintf(&showdumpout,"%-79s\n",fault_type);
   Dex32SetTextBackground(showdumpout,BLACK);
   DDLprintf(&showdumpout,"Faulting process                       :%s\n",current_process->name);
   DDLprintf(&showdumpout,"Process ID                             :%d\n",current_process->processid);
   
   DDLprintf(&showdumpout,"\n============ <Memory Access Information>=================\n");
   DDLprintf(&showdumpout,"Tried to access invalid memory location: 0x%x\n",location);
   DDLprintf(&showdumpout,"Page directory entry at that address is: 0x%x\n",direntry);
   DDLprintf(&showdumpout,"Page table entry at that address is    : 0x%x\n",pageentry);
   DDLprintf(&showdumpout,"Address of faulting instruction is     : 0x%x\n",current_process->regs.EIP);
   DDLprintf(&showdumpout,"Kernel Page Directory : 0x%x  Process Page Directory: 0x%x",pagedir1,current_process->regs.CR3);

   DDLprintf(&showdumpout,"\n============ <Register values at time of fault>==========\n");
   DDLprintf(&showdumpout,"EAX=0x%x EBX=0x%x ECX=0x%x EDX=0x%x ",current_process->regs.EAX,
         current_process->regs.EBX,current_process->regs.ECX,current_process->regs.EDX);

   DDLprintf(&showdumpout,"EBP=0x%x EDI=0x%x \nESI=0x%x ESP=0x%x ",
         current_process->regs.EBP,current_process->regs.EDI,
         current_process->regs.ESI,current_process->regs.ESP);
   
   DDLprintf(&showdumpout,"CS=0x%x DS=0x%x ES=0x%x SS=0x%x ",
      current_process->regs.CS,current_process->regs.DS,current_process->regs.ES,
      current_process->regs.SS);
   
   DDLprintf(&showdumpout,"FS=0x%x GS=0x%x \n",current_process->regs.FS,current_process->regs.GS);
   
   exc_dumpflags(&showdumpout,current_process->regs.EFLAGS);
    
   DDLprintf(&showdumpout,"\n\nAPI call information\n");
   DDLprintf(&showdumpout,"=========================================================\n"); 
   DDLprintf(&showdumpout,"Process Top of Heap location: 0x%x\n",(DWORD)current_process->knext);
   DDLprintf(&showdumpout,"Kernel Top of Heap location: 0x%x\n",ktopheap);
   DDLprintf(&showdumpout,"Last system calls:(1) : 0x%x ,(2-last): 0x%x\n",
      current_process->cursyscall[0],current_process->cursyscall[1]);
   DDLprintf(&showdumpout,"Context information:  device # %d(%s), function # %d(%s)\n",
             current_process->context,
             devmgr_getname(current_process->context),
             current_process->function,
             get_function_name(current_process->context,current_process->function));


   if (current_process->op_success==1)    DDLprintf(&showdumpout,"syscall terminated normally\n");
      else
      {
         DDLprintf(&showdumpout,"Fault occured during system call.\n");
      };

   /* Always copy a compact dump to COM1 so headless qemu tests can see
      faults. Do not wait for a key — kb_pause() hangs -display none. */
   {
      char line[160];
      sprintf(line, "%s process=%s eip=0x%x loc=0x%x\n",
              fault_type, current_process->name,
              current_process->regs.EIP, location);
      serial_puts(line);
   }

   Dex32SetActiveDDL(beforeout);
  
   #else
      printf("%s\n",fault_type);
      printf("faulting process                       :%s\n",current_process->name);
      printf("Tried to access invalid memory location: 0x%x\n",location);
      printf("Page directory entry at that address is: 0x%x\n",pageentry);
      printf("Address of faulting instruction is     : 0x%x\n",current_process->regs.EIP);
   #endif
   
   stopints();

   enable_taskswitching();
   
};

// the core function that handles page faults, it is also
// dirctly linked to the virtual memory manager of the operating system
DWORD pagefaulthandler(DWORD location, DWORD fault_info, unsigned long rip)
  {
   DWORD ret,mm,i;
   static volatile int pf_busy = 0;
   stopints();
   pfoccured=1;//set the pfoccured register of the task scheduler

#ifdef __x86_64__
   if (pf_busy) {
      serial_puts("PF64: re-entered -> halt\n");
      while (1) {}
   }
   pf_busy = 1;
   if (!current_process || !(DWORD)(uintptr)current_process->pagedirloc) {
      serial_puts("PF64: bad current_process/pagedirloc -> halt\n");
      while (1) {}
   }
   mm=getphys(location,current_process->pagedirloc);
   /* Not present: lazily map a private frame for the user window / ELF image
      instead of dumping (dumping under the user CR3 is what double-faulted). */
   if ((mm & PG_PRESENT) == 0) {
      DWORD *pd = (DWORD *)(uintptr)current_process->pagedirloc;
      if (pd && pd != pagedir1
          && (unsigned)location >= 0x400000u
          && (unsigned)location < 0x10000000u) {
         u64 *fr = userpd_map_page((u64 *)(uintptr)pd,
                                   (unsigned long long)(unsigned)location,
                                   (unsigned long)(PG_WR | PG_USER));
         if (fr) {
            __asm__ __volatile__("invlpg (%0)" :: "r" ((unsigned long)(unsigned)location) : "memory");
            pf_busy = 0;
            return 0;
         }
         serial_puts("PF64: userpd_map_page failed (pool empty?)\n");
      }
      {
         char line[180];
         sprintf(line,
                 "PF64: not-present cr2=0x%x rip=0x%lx err=0x%x proc=%s mm=0x%x\n",
                 (unsigned)location, rip, (unsigned)fault_info,
                 current_process->name ? current_process->name : "?",
                 (unsigned)mm);
         serial_puts(line);
      }
      pf_busy = 0;
      exc_showdump(location, PAGE_FAULT, mm);
      exc_recover();
      while (1) {}
   }
   pf_busy = 0;
#else
   (void)rip;
   mm=getphys(location,current_process->pagedirloc);
#endif
   
   if (mm&PG_DEMANDLOAD)
         {
             //A page marked as demand paged has been called so
             //we allocate a physical frame to this page
             DWORD pg,pageadr=(DWORD)mempop();

           //  printf("demand paged %s..",itoa(location,temp,16));
             if (pageadr==0) //we're out of physical frames, find a way to get one
             pageadr=obtainpage(); //call the VMM to get a page

             pg=(DWORD)getvirtaddress((DWORD)current_process->pagedirloc);

             maplineartophysical2(pg,location,
             pageadr,PG_PRESENT | PG_USER | PG_WR);
   
             pg=(DWORD)getvirtaddress(pageadr);
             memset(pg,0,0x1000);
                
             //Finished allocation, let the task scheduler take over
             setattb(PF_TSS,0x89); //reset the TSS attribute
             taskswitch();
        ;};
   
   // a copy-on-write page has been written to, we therefore duplicate this page
   // so that forked processes have their own unique set of data.     
   if (mm&PG_COPYWRITE) 
        {
                DWORD destpg,pdirpg;
                DWORD pageadr= (DWORD) mempop(); //allocate new memory
                char pagebuf[0x1000];
                if (pageadr==0) //we're out of physical frames, find a way to get one
                //call the VMM to get a page, "assume" it works
                pageadr=obtainpage(); 

                                
                //obtain a virtual memory address for this physical address
                destpg=(DWORD)getvirtaddress(pageadr); 
                                                       
                //copy the content of the copy-on-write page to the new page         
                memcpy(destpg, location&0xFFFFF000 ,0x1000);
                
                //obtain a virtual address for the page directory
                pdirpg=(DWORD)getvirtaddress((DWORD)current_process->pagedirloc);
                
                //update the page directory of the process to reflect this change
                maplineartophysical2(pdirpg,location,
                pageadr,PG_PRESENT | PG_USER | PG_WR);

                #ifdef DEBUG_FORK                 
                printf("COPY ON WRITE DETECTED.\n");
                #endif
                
                setattb(PF_TSS,0x89); //reset the TSS attribute
                taskswitch();                
        };
   //show register dump and other important information
   exc_showdump(location,PAGE_FAULT,mm);        
   //Try to recover from the fault
   exc_recover();
    
   while (1) {};
   startints();
  };

  
//This function is called when DEX is trying to recover from a fault
void exc_recover(){
   if (current_process->processid==0){
      printf("dex32_kernel: kernel mode page fault.\n");
      printf("recovery mode active. system may become unstable.\n");
      printf("System Halted\n");
      while (1);
   }else{
      printf("dex32_kernel: user mode page fault.\n");
      printf("recovery mode active. system may become unstable.\n");
      printf("shutting down application..\n");
      /* Non-zero so waiters can distinguish crash from clean exit(0). */
      dex32_child_faulted = 1;
      exit(1);
      startints();
   };
};

