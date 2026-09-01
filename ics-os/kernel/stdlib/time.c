DWORD time_count = 0,  //used to store the number of seconds since dex was booted
aux_time2=0;   //since the OS has the timer set to interrupt 200 times a second
               //an auxillary counter is required so that it increments time_count
               //if it reaches 200
               
int time_monthdays[]= {0,31,59,90,120,151,181,212,243,273,304,334,365}; 

//the tme returned by the timer chip is in BCD, so we have to
//perform some conversions to binary
DWORD bcdtobinary(DWORD b)
  {

   DWORD x= b & 0xff,c,r;
   r = x & 0xF;
   c = x >> 4;
   return (c*10+r);

  ;};

char *getmonthname(int month,char *str)
  {
     switch (month)
       {
         case 1 : strcpy(str,"January"); break;
         case 2 : strcpy(str,"Febuary"); break;
         case 3 : strcpy(str,"March");break;
         case 4 : strcpy(str,"April");break;
         case 5 : strcpy(str,"May");break;
         case 6 : strcpy(str,"June");break;
         case 7 : strcpy(str,"July");break;
         case 8 : strcpy(str,"August");break;
         case 9 : strcpy(str,"September");break;
         case 10: strcpy(str,"October");break;
         case 11: strcpy(str,"November");break;
         case 12: strcpy(str,"Decemeber");break;
       };
    return str;
  };

char *datetostr(dex32_datetime *d,char *str)
  {
     char temp1[20],temp2[20],temp3[20];
     sprintf(str,"%s/%s/%s",itoa(d->month,temp1,10),
            itoa(d->day,temp2,10),itoa(d->year,temp3,10));
     return str;
  };

void getdatetime(dex32_datetime *d) //gets the date nd time
  {
     DWORD x;

     //seconds
     outportb(0x70,0);
     //delay(1);
     x=inportb(0x71);
     d->sec=bcdtobinary(x); //convert to binary

     //minutes
     outportb(0x70,2);
     //delay(1);
     x=inportb(0x71);
     d->min=bcdtobinary(x);

     //hours
     outportb(0x70,4);
     //delay(1);
     x=inportb(0x71);
     d->hour=bcdtobinary(x);

     outportb(0x70,0x7);
     //delay(1);
     x=inportb(0x71);
     d->day=bcdtobinary(x);

     outportb(0x70,0x8);
     //delay(1);
     x=inportb(0x71);
     d->month=bcdtobinary(x);

     outportb(0x70,9);
     //delay(1);
     x=inportb(0x71);
     d->year=bcdtobinary(x);
     if (d->year<80) d->year+=2000; //adjust for the year 2000
  };
  
//returns time in milliseconds
DWORD time_gettime()
{
return time_count;
};

//returns time in milliseconds
DWORD getprecisetime()
{
 return (time_count*100+(aux_time2/2));
};

int time()
   {
     int totaldays = (time_systime.year - 1970)*365;
     int totalseconds,totalminutes,totalhours;
     if (time_systime.year%4 !=0 || time_systime.month>2)
     totaldays+=time_systime.year/4;
        else
     { 
     totaldays+=(time_systime.year/4) - 1;   
     };
     
     if (time_systime.month>2&&time_systime.year%4==0) totaldays+=1;
     totaldays+=time_monthdays[time_systime.month-1];
     totaldays+=time_systime.day;
     totalhours = totaldays*24 + time_systime.hour;
     totalminutes = totalhours*60 + time_systime.min;
     totalseconds = totalminutes * 60;
     totalseconds += time_systime.sec;
     return totalseconds;
   };    

int time_getmycputime()
{
    return current_process->totalcputime;
};
//increments the system time by one millisecond
void time_incrementtime()
{
    time_systime.adj++;
    if (time_systime.adj>context_switch_rate/100)
      {
       time_systime.ms ++;
       time_systime.adj = 0;
      }; 
    if (time_systime.ms>=100)
      {
         time_systime.ms = 0;
         time_systime.sec++;
         if (time_systime.sec>=60)
           {
               time_systime.sec=0;
               time_systime.min++;
               if (time_systime.min>=60)
               {
               time_systime.min=0;
               time_systime.hour++;
               if (time_systime.hour>=24)
                   time_systime.hour=0;
               };
           };
      };
};
   
/* Diagnostic spin watchdog for selfhost-stage1.  In cooperative mode the
    timer no longer preempts (schedule_from_timer returns early), so a user
    process that never calls taskswitch() holds the CPU forever.  time_handler()
    still runs on every tick, so track the current process and report any that
    keeps the CPU for more than a few seconds without a voluntary switch, along
    with the running syscall total (delta between reports = syscall rate) and the
    last two syscalls it issued.  Diagnostic-only: never changes scheduling. */
 /* Wide interrupt-frame window captured by irqwrap.S timerwrapper at
     [rsp+0..240] (31 qwords) plus the pre-PUSH_ALL RSP (wd_krsp).  The watchdog
     filters for code-range values so the live user RIP (user ELF range) or the
     kernel return addresses (kernel text range) stand out for symbolization. */
  volatile unsigned long wd_frame[31];
  volatile unsigned long wd_krsp = 0;

  /* 0 = not a code pointer, 1 = kernel text, 2 = user ELF (non-PIE) code. */
  static int wd_kind(unsigned long v)
    {
     if (v >= 0x100000 && v < 0x300000)
        return 1;
     if (v >= 0x400000 && v < 0x1800000)
        return 2;
     return 0;
    };

  static void selfhost_spin_watchdog(void)
    {
     extern volatile int selfhost_cooperative_ready;
     extern volatile unsigned long diag_sc_count;
     extern int smp_cpu_id(void);
     static PCB386 *wd_last;
     static unsigned long wd_ticks;
     unsigned i;

     if (!selfhost_cooperative_ready)
        return;
     if (smp_cpu_id() != 0)
        return;

     if (current_process != wd_last)
        {
         wd_last = current_process;
         wd_ticks = 0;
        };
     wd_ticks++;
     if (wd_ticks < 500)
        return;
     if ((wd_ticks - 500) % 2000 != 0)
        return;

     static int wd_raw_done;

      if (current_process)
         printf("WATCHDOG: pid=%d '%s' no-yield=%lu sc_total=%lu last_sc=%02x/%02x krsp=0x%lx rip=0x%lx\n",
                current_process->processid, current_process->name,
                wd_ticks,
                diag_sc_count,
                (unsigned)current_process->cursyscall[0],
                (unsigned)current_process->cursyscall[1],
                wd_krsp, wd_frame[15]);
      else
         printf("WATCHDOG: idle no-yield=%lu sc_total=%lu krsp=0x%lx rip=0x%lx\n",
                wd_ticks, diag_sc_count, wd_krsp, wd_frame[15]);

      /* One-shot raw dump of the full frame window + kernel C stack so the live
          RIP and the exact stack layout can be read without range filtering. */
       if (!wd_raw_done && current_process)
         {
          wd_raw_done = 1;
          for (i = 0; i < 31; i++)
             printf("  RAW f[%lu]=0x%lx\n", (unsigned long)(i*8), wd_frame[i]);
          if (wd_krsp >= 0x100000 && wd_krsp < 0x1000000)
             for (i = 0; i < 32; i++)
                printf("  RAW k[%lu]=0x%lx\n", (unsigned long)(24 + i*8),
                       *(volatile unsigned long *)(wd_krsp + 24 + i*8));
         }

       /* One-shot per-process PCB/context dump: does the SAVED ctx.rip match the
          wild live RIP, and what does the legacy regs.EIP hold? */
      static PCB386 *wd_dumped;
       if (current_process && current_process != wd_dumped)
         {
          PCB386 *p = current_process;
          wd_dumped = p;
          printf("PCBDUMP pid=%d '%s':\n", p->processid, p->name);
          printf("  ctx.rip=0x%lx ctx.rsp=0x%lx ctx.rflags=0x%lx ctx.cs=0x%lx ctx.ss=0x%lx ctx.cr3=0x%lx\n",
                 p->ctx.rip, p->ctx.rsp, p->ctx.rflags, p->ctx.cs, p->ctx.ss, p->ctx.cr3);
          printf("  regs.EIP=0x%lx regs.ESP=0x%lx regs.CS=0x%lx status=0x%lx pagedir=0x%lx\n",
                 (unsigned long)p->regs.EIP, (unsigned long)p->regs.ESP,
                 (unsigned long)p->regs.CS, p->status, (unsigned long)p->pagedirloc);
          if (p->ctx.rsp >= 0x100000 && p->ctx.rsp < 0x1000000)
             printf("  [rsp+120]=0x%lx [rsp+112]=0x%lx [rsp+136]=0x%lx\n",
                    *(volatile unsigned long *)(p->ctx.rsp + 120),
                    *(volatile unsigned long *)(p->ctx.rsp + 112),
                    *(volatile unsigned long *)(p->ctx.rsp + 136));
         }

      for (i = 0; i < 31; i++)
         if (wd_kind(wd_frame[i]))
            printf("  FR f[%lu]=0x%lx %s\n", (unsigned long)(i*8),
                   wd_frame[i], wd_kind(wd_frame[i])==1 ? "K" : "U");
      if (wd_krsp >= 0x100000 && wd_krsp < 0x1000000)
         {
         for (i = 0; i < 32; i++)
            {
             unsigned long v = *(volatile unsigned long *)(wd_krsp + 24 + i*8);
             if (wd_kind(v))
                printf("  KS k[%lu]=0x%lx %s\n", (unsigned long)(24 + i*8),
                       v, wd_kind(v)==1 ? "K" : "U");
            }
        }
    };

 //the timer handler used by the task switcher
 void time_handler()
   {
    //update the real-time clock
   //DEX32 is programmed to switch process every
   // 1/200 of a second so we use a counter that counts
   //up to 200 and then increments time_count which
   //holds the time elasped in seconds since the
   //system has started
   
   aux_time2++;
   
   
   if (aux_time2>=context_switch_rate)
         {
             aux_time2=0;
             time_count++;

             //synchronize with the clock every 10 minutes
             if (time_systime.min%10==0)
                 getdatetime(&time_systime);
         };

   ticks++;
    time_incrementtime();
    selfhost_spin_watchdog();
    
    /* Floppy motor timeout and PIC EOI are BSP-only. APs use the LAPIC. */
   {
      extern int smp_cpu_id(void);
      if (smp_cpu_id() == 0) {
         fdctimer();
         outportb(0x20,0x20);
      }
   }
   {
      extern void lapic_eoi(void);
      extern volatile unsigned int *lapic_mmio;
      if (lapic_mmio)
         lapic_eoi();
   }
;};

void cpu_idle(void)
 {
   /* Interrupts must be on so the PIT/keyboard can wake us. */
   asm volatile ("sti; hlt");
 };

//delays the execution of a program for a specified number of milliseconds
void delay(DWORD w)
 {
   DWORD t1, cpuflags;
   storeflags(&cpuflags);
   stopints();
   
   t1 = ticks+w*2;
   restoreflags(cpuflags);
      
   while (ticks<t1)
      cpu_idle();
 };

//sets the rate of context switch
//It is best to set the value between 100-300 to prevent
//erratic behavior

void dex32_set_timer(DWORD rate)
{
    WORD time_val;
    BYTE time_val_high,time_val_low;
    DWORD flags;
    storeflags(&flags);
    stopints(); //stop interrupts

    time_val= 1193180 / rate;
    time_val_low = time_val & 0xFF;
    time_val_high = time_val >> 8;

    outportb(0x43,0x36); //tell which timer to reprogram
    outportb(0x40,time_val_low);
    outportb(0x40,time_val_high);
    restoreflags(flags);
};


void time_init()
{
    //update system time
    getdatetime(&time_systime);
};
