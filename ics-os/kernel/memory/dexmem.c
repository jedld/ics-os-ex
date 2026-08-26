/*
  Name: dex low-level memory management library
  Copyright: 
  Author: Joseph Emmanuel DL Dayo
  Date: 02/03/04 18:06
  Description: This module handles everything that has to do
  with memory, except the high-level memory functions like malloc....
*/




/* Stop Interrupts */
inline void stopints(){
  asm ("cli");
}

/*start Interrupts */

inline void startints(){
  asm("sti");
};

inline void wbinvd(){
  asm ("wbinvd");
};

inline void hlt(){
  asm ("hlt");
};

extern void refreshpages();

/*Displays memory information provided by the GRUB bootloader*/
void mem_interpretmemory(mmap *map,int size){
   int i;
   for (i=0; i <(size/sizeof(mmap)); i++){
      DWORD base = map[i].base_addr_low;
      DWORD base_end = base + map[i].length_low;
      printf("region: 0x%X - 0x%X . ", base, base_end);
      if (map[i].type==1) 
         printf("FREE\n");
      else
         printf("RESERVED.\n");
   }; 
};

/*using the memory map provided by grub, create the stack of physical frames.
  On modern machines the map can describe many gigabytes; a 32-bit kernel can
  only use the first 4GB and we cap usable RAM at 256MB so the free-page stack
  at 0x200000 cannot overrun the identity-mapped region.*/
#define MEM_MIN_FRAME   0x00300000
#define MEM_MAX_PHYS    0x10000000

/* Dedicated per-process user page-directory frame pool (x86_64). See the
   userpd_* section below for why the pool must stay BELOW the identity-
   mapped user window [0x08000000,0x10000000). */
#define UPD_POOL_BASE   0x06000000
#define UPD_POOL_TOP    0x08000000
#define USERPD_MAXFRAMES ((UPD_POOL_TOP - UPD_POOL_BASE) >> 12)

DWORD mem_detectmemory(mmap *grub_meminfo , int size ){
   DWORD mem_size = 0;
   unsigned char *cursor;
   unsigned char *end;
   volatile DWORD *fps = (volatile DWORD *)0x200000UL;

   fps[0]=0;

#ifdef __x86_64__
   {
      DWORD page;
      DWORD count = 0;
      (void)grub_meminfo;
      (void)size;
      /* Seed free frames up to 128MiB (QEMU default). Skip kernel stacks,
         heap, the dedicated per-process userpd frame pool, and fixed user
         windows (ELF/stack/heap) that stay identity-mapped —
         maplineartophysical2 is still 32-bit and must not touch PML4. */
      for (page = 0x00400000; page < 0x08000000; page += 0x1000) {
         if (page >= 0x02800000 && page < 0x02900000)
            continue;
         if (page >= 0x03000000 && page < 0x03400000)
            continue;
         /* Dedicated per-process user page-directory frame pool
            (see userpd_* below): owned by upop()/upush(), not mempop(). */
         if (page >= UPD_POOL_BASE && page < UPD_POOL_TOP)
            continue;
         /* userspace ELF load window */
         if (page >= 0x00400000 && page < 0x00800000)
            continue;
         /* syscall stack / user heap / user stack */
         if (page >= 0x09000000 && page < 0x0C000000)
            continue;
         count++;
         fps[count] = page;
         mem_size += 0x1000;
      }
      fps[0] = count;
      stackbase = (DWORD *)0x200000UL;
      totalpages = count;
      return mem_size;
   }
#else
   stackbase[0]=0;
   if (grub_meminfo == 0 || size <= 0)
      return 0;

   cursor = (unsigned char*)grub_meminfo;
   end = cursor + size;
#endif

   while (cursor + sizeof(mmap) <= end){
      mmap *entry = (mmap*)cursor;
      DWORD base, length, base_end, page;

      if (entry->size == 0)
         break;

      /* Skip regions above 4GB (base_addr_high) and reserved RAM. */
      if (entry->type == 1 && entry->base_addr_high == 0){
         base = entry->base_addr_low;
         length = entry->length_low;

         if (base >= MEM_MAX_PHYS)
            length = 0;
         else if (length > MEM_MAX_PHYS - base)
            length = MEM_MAX_PHYS - base;

         if (length){
            base_end = base + length;
            for (page = base; page < base_end; page += 0x1000){
               if (page >= MEM_MIN_FRAME && page + 0x1000 > page){
                  stackbase[0]++;
                  stackbase[stackbase[0]] = page;
               }
            }
            mem_size += length;
         }
      }

      cursor += entry->size + 4;
   }

   totalpages = stackbase[0];
   return mem_size;
};



DWORD *mempop(){
   DWORD *ret;
   if (stackbase[0]==0) 
      return 0; //no more free pages available!
   ret=(DWORD*)stackbase[stackbase[0]];
   stackbase[0]--;
   return ret;
};

void mempush(DWORD mem){
   //make sure that the physcial memory location pushed is within
   //the range of the computer's physical memory
   if (stackbase+0x100000>=mem<memamount-0x1000){
      stackbase[0]++;
      stackbase[stackbase[0]]=mem;
   }else{ 
      char temp[255]; 
      printf("memory manager: An invalid value (%s) was tried to be added to the\n");
      printf("memory manager: free physical pages list.\n");    
   };
};

void clearpagetable(DWORD *pagetable){
   DWORD i;
   for (i=0;i<4096;i++)
      pagetable[i]=0;
};

/* This function maps the linear address to a physical address:
   The function automatically allocates a page for a page table
   the bottom 12 bits of the linear address is discarded since
   it is not used by the mapper*/
void maplineartophysical(unsigned int *pagedir, /*the location of the page directory*/
                              unsigned int linearaddr, /*the paged aligned linear address requested*/
                              unsigned int physical,   /*the paged aligned physical address to map to*/
									 unsigned int attribute /*used to specify the page attributes to be applied*/
					    ){
#ifdef __x86_64__
   /* First 4GiB are identity-mapped with 2MiB pages at boot. */
   (void)pagedir; (void)linearaddr; (void)physical; (void)attribute;
   return;
#else
   unsigned int pagedirindex,pagetableindex,*pagetable;
   /*get the index of the page directory and the pagetable respectively*/
   pagedirindex= linearaddr >> 22;
   pagetableindex= (linearaddr  & 0x3FFFFF) >> 12;
   /*get the location of the page table and mask the first 12 bits*/
   pagetable=(unsigned int*)(pagedir[pagedirindex] & 0xFFFFF000);
   if (pagetable==0){  /*there is no entry?*/
      /*map a new memory location*/
      pagedir[pagedirindex]=(DWORD) mempop();
      pagetable=(unsigned int*)pagedir[pagedirindex];
      /*clear the locations of the page table to zero*/
      memset(pagetable,0,4096);
      /*set the present bit of the pagetable dir entry*/
      pagedir[pagedirindex]=pagedir[pagedirindex] | 1 | PG_USER | PG_WR;
   };
   physical=(physical & 0xFFFFF000) | attribute;
   pagetable[pagetableindex]=physical;
      /*done!*/
#endif
};

DWORD tlb_address;
extern void invtlb();

/*same as map linear to physical except that it does its tricks without
 modifying the paging bit   */
int maplineartophysical2(unsigned int *pagedir, /*the location of the page directory*/
                              unsigned int linearaddr, /*the paged aligned linear address requested*/
                              unsigned int physical,   /*the paged aligned physical address to map to*/
									 unsigned int attribute /*used to specify the page attributes to be applied*/
					    ){
#ifdef __x86_64__
   /* Boot identity-maps the low 4GiB with 4-level tables. The legacy
      2-level walker below would corrupt PML4 — treat as already mapped. */
   (void)pagedir;
   (void)linearaddr;
   (void)physical;
   (void)attribute;
   return 0;
#else
   unsigned int pagedirindex,pagetableindex,*pagetable;
   DWORD pg;
   DWORD *kicker=(DWORD*)SYS_PAGEDIR2_VIR;
      
	/*get the index of the page directory and the pagetable respectively*/
	pagedirindex= linearaddr >> 22;
	pagetableindex= (linearaddr  & 0x3FFFFF) >> 12;
	  
	/*get the location of the page table and mask the first 12 bits*/
	pg=(pagedir[pagedirindex] & 0xFFFFF000);
	  
	if (pg==0){  /*there is no entry?*/
      /*map a new memory location*/
      pagedir[pagedirindex]=(DWORD) mempop();
      kicker[4]=(pagedir[pagedirindex]&0xFFFFF000) | 1;
                
      refreshpages();
                
      pagetable=(DWORD*)SYS_PAGEDIR4_VIR;

      tlb_address = pagetable;
      invtlb();                
				
      /*clear the locations of the page table to zero*/
		memset(pagetable,0,4096);
		/*set the present bit of the pagetable dir entry*/
		pagedir[pagedirindex] = pagedir[pagedirindex] | 1 | PG_USER | PG_WR;
		refreshpages();
				
		pg = (pagedir[pagedirindex] & 0xFFFFF000);
   };
             
   kicker[4]=pg | 1;
   refreshpages();
        

   pagetable=(DWORD*)SYS_PAGEDIR4_VIR;

   physical = (physical & 0xFFFFF000) | attribute;
   pagetable[pagetableindex]=physical;

   refreshpages();
    
   tlb_address = linearaddr;
   invtlb();
    
   /*done!*/
   return 0;
#endif
};


//quickly gives a physical address a corresponding virtual address
//--used for modifying page tables without disabling paging
DWORD getvirtaddress(DWORD physicaladdr){
#ifdef __x86_64__
   /* Boot identity-maps the first 4GiB; physical == virtual. */
   return physicaladdr;
#else
   DWORD *kicker=(DWORD*)SYS_PAGEDIR2_VIR; //obtain the aux pagetable
   kicker[2]=physicaladdr | 1;
   refreshpages();
   return SYS_PAGEDIR3_VIR;
#endif
};

DWORD getvirtaddress2(DWORD physicaladdr,DWORD hdl){
#ifdef __x86_64__
   (void)hdl;
   return physicaladdr;
#else
   DWORD *kicker=(DWORD*)SYS_PAGEDIR2_VIR; //obtain the aux pagetable
   kicker[hdl]=physicaladdr | 1;
   refreshpages();
   return SYS_PAGEDIR_VIR+hdl;
#endif
};


DWORD xmaplineartophysical(const DWORD linearmemory,const DWORD physicalmemory,
                              DWORD *pagedir,const DWORD attb){ //ATOMIC
   DWORD w=linearmemory;
   DWORD dirindex=(w&0xFFC00000) >> 22;
   DWORD pageindex=(w&0x3FF000) >> 12;
   DWORD *pagetbl;
   DWORD flags;
   dex32_stopints(&flags);
   disablepaging();
   if (pagedir[dirindex]&PG_PRESENT==0){ //no page table allocated?
      pagetbl=mempop();
      if (pagetbl==0) {
         dex32_restoreints(flags);
         enablepaging();
         return 0;
      };
      pagedir[dirindex]=(DWORD)pagetbl | 1;
   };
        
   if (pagedir[dirindex]&1){
      pagetbl=(DWORD*)(pagedir[dirindex]&0xFFFFF000);
      pagetbl[pageindex]=physicalmemory | (attb&0xFFF);
       
      dex32_restoreints(flags);
      enablepaging();
      return 1;
   }
      
   dex32_restoreints(flags);
   enablepaging();
   return 0;
};

int ints_enabled=1;

void dex32_stopints(DWORD *flags){
   storeflags(flags);
   stopints();
};

void dex32_restoreints(DWORD flags){
   restoreflags(flags);
};


DWORD getphys(DWORD vaddr,DWORD *pagedir){
#ifdef __x86_64__
   (void)pagedir;
   return (vaddr & 0xFFFFF000) | 1 | PG_WR;
#else
   DWORD dirindex=(vaddr&0xFFC00000) >> 22;
   DWORD pageindex=(vaddr&0x3FF000) >> 12;
   DWORD *pagetbl;
   DWORD ret;
   DWORD *pg;

   pg=(DWORD*)getvirtaddress((DWORD)pagedir);
   if (pg[dirindex]&1==0)
      return 0;
   pagetbl=(DWORD*)(pg[dirindex]&0xFFFFF000);
   pg=(DWORD*)getvirtaddress((DWORD)pagetbl);
   ret=pg[pageindex];
   return ret;
#endif
};

DWORD getpagetablephys(DWORD vaddr,DWORD *pagedir){
   DWORD dirindex=(vaddr&0xFFC00000) >> 22;
   DWORD *pg;
   pg=(DWORD*)getvirtaddress((DWORD)pagedir);
   return pg[dirindex];
};

void dex32_freeuserpagetable(DWORD *pgd){  //ATOMIC function
   /* Process page directories may drop everything above 4MB (userspace).
      The kernel page directory (pagedir1) identity-maps 0-16MB for the
      kernel image and heap. ELF/PE loaders dual-map user programs into
      pagedir1 then call this; they must not free PDE 1-3 (4-16MB) or the
      next disk read / malloc above 4MB hangs. */
   DWORD userstart, userend  =0xC0000000 >> 22;
   DWORD auxstart=0xFFC00000 >> 22;
   DWORD *pagedir,cpuflags;
   DWORD *pagetbl,address;
   DWORD i;
   DWORD pages=0;
   storeflags(&cpuflags);
   stopints();

   if (pgd == pagedir1)
      userstart = 0x01000000 >> 22; /* preserve 0-16MB identity map */
   else
      userstart = (DWORD)userspace >> 22;
     
   pagedir =(DWORD*)getvirtaddress((DWORD)pgd);
   for (i=userstart;i<userend;i++)
      if (pagedir[i]&1){ //check if present
         mempush(pagedir[i]&0xFFFFF000);
         pages++;
         pagedir[i]=0;
      };
             
   if (pagedir[auxstart]&1&&(pgd!=pagedir1)){
      mempush(pagedir[auxstart]&0xFFFFF000);
      pages++;
      pagedir[auxstart]=0;
   };
   restoreflags(cpuflags);
   #ifdef MEM_LEAK_CHECK
   printf("freeuserpagetable() frees %d pages.\n",pages);
   #endif
};


void freeuserheap(DWORD *pagedir){
   DWORD start=0xA0000000,end=0xB0000000;
   DWORD i;
   for (i=start;i<end;i++){
      DWORD w=getphys(i,pagedir);
      if (w&1){
         mempush(w&0xFFFFF000);
         maplineartophysical2(pagedir,i,0,0);
      };
   };
};


void freelinearloc(void *linearmemory,DWORD *pagedir){  //ATOMIC function
   DWORD w=(DWORD)linearmemory;
   DWORD dirindex=(w&0xFFC00000) >> 22;
   DWORD pageindex=(w&0x3FFFFF) >> 12;
   DWORD *pagetbl,address,flags;
   char temp[255];

   dex32_stopints(&flags); 
   // disablepaging();
   address = getphys(linearmemory,pagedir);
   /* pagetbl=(DWORD*)(pagedir[dirindex]&0xFFFFF000);
   address=pagetbl[pageindex];*/
   if (address&1)
      mempush(address&0xFFFFF000);
   //pagetbl[pageindex]=0;
   // enablepaging();
   dex32_restoreints(flags);
};

//frees multiple pages and returns them back to the stack
void freemultiple(void *linearmemory,DWORD *pagedir,DWORD pages)
  {
  int i;
  #ifdef MEM_LEAK_CHECK
  printf("freemultiple() frees %d pages.\n",pages);
  #endif
  for (i=0;i<pages;i++)
    {
      freelinearloc(linearmemory,pagedir);
      linearmemory=(void*)(linearmemory+0x1000);
    };
  };

DWORD getlinearloc(void *linearmemory,DWORD *pagedir)  //ATOMIC function
   { 
     DWORD w=(DWORD)linearmemory,ret=0,*pg;
     DWORD dirindex=(w&0xFFC00000) >> 22;
     DWORD pageindex=(w&0x3FFFFF) >> 12;
     DWORD *pagetbl,address,flags;
     char temp[255];

     dex32_stopints(&flags);
     pg=(DWORD*)getvirtaddress((DWORD)pagedir); //convert to a virtual address so that
               
     pagetbl=(DWORD*)(pg[dirindex]&0xFFFFF000);
     if (pagetbl!=0)
     {
          pg=(DWORD*)getvirtaddress((DWORD)pagetbl); //convert to a virtual address so that
          address=pg[pageindex];
          if (address&1)
          ret = 1;
     }
     else
     ret =0;    
     
     dex32_restoreints(flags);
     return ret;
   };


//Determines the amount of physical pages commited based on a
//given linear memory
DWORD getmultiple(void *linearmemory,DWORD *pagedir,DWORD pages)
  {
  int i;
  DWORD total=0;
  
  for (i=0;i<pages;i++)
    {
      total+=getlinearloc(linearmemory,pagedir);
      linearmemory=(void*)(linearmemory+0x1000);
    };
    return total;
  };


void setpageattb(DWORD *pagedir,DWORD vaddr,DWORD attb)
  {
    DWORD pg,phys;
    phys=getphys(vaddr,pagedir);
    pg=(DWORD)getvirtaddress((DWORD)pagedir);

    maplineartophysical2(pg,vaddr,
    phys,attb);
          //   printf("ok\n");

  };


void *dex32_setpageattb(DWORD virtualaddr,DWORD pages,DWORD *pagedir,DWORD pattb)
    {
     int i;
     char temp[255],temp2[255];
     void *ret=(void*)virtualaddr;
     DWORD *pg;
     for (i=0;i<pages;i++)
        {
          DWORD pageadr;
          setpageattb(pagedir,virtualaddr,pattb);
          if (current_process->accesslevel==ACCESS_SYS)
          setpageattb(pagedir1,virtualaddr,pattb);
          virtualaddr+=0x1000;
         };
     refreshpages();
     return ret;
    };

void *dex32_setpageattbblock(DWORD virtualaddr,int amt,DWORD *pagecount,DWORD *pagedir,DWORD attb)
   {
     int pages=(amt/4096)+1;
     char *ret=0;
     if (amt==0) pages=1;
        else
     if (amt%4096==0) pages=amt/4096;
     ret=dex32_setpageattb(virtualaddr,pages,pagedir,attb);
     *pagecount=pages;
     return ret;
   };

//returns the total number of free physical pages on the system

DWORD dex32_getfreepages()
   {
       return totalpages; //the totalpages global variable holds the number of pages left

   };



void setgdtentry(DWORD index,void *base,gdtentry *t,DWORD limit,
           BYTE attb1,BYTE attb2)
   {
      DWORD b=(DWORD)base;
      t->lowaddr=b;
      t->lowaddr2=b >> 16;
      t->limit=limit;
      t->att1=attb1;
      t->att2=attb2;
      t->highaddr=b >> 24;
   };

#define RING0_TSS 0x89

WORD addgdt(DWORD base,DWORD limit,BYTE attb1,BYTE attb2)
  {
    int index=totalgdtentries;
    dex_gdtbase[index].lowaddr=base;
    dex_gdtbase[index].lowaddr2=base >> 16;
    dex_gdtbase[index].limit=limit;
    dex_gdtbase[index].att1=attb1;
    dex_gdtbase[index].att2=attb2 | ((limit >> 8)&0xF);
    dex_gdtbase[index].highaddr=base >> 24;
    return (index << 3);
  };


void setgdt(WORD sel,DWORD base,DWORD limit,BYTE attb1,BYTE attb2)
  {
#ifdef __x86_64__
    (void)sel; (void)base; (void)limit; (void)attb1; (void)attb2;
    return;
#else
    sel=sel >> 3;
    dex_gdtbase[sel].lowaddr=base;
    dex_gdtbase[sel].lowaddr2=base >> 16;
    dex_gdtbase[sel].limit=limit;
    dex_gdtbase[sel].att1=attb1;
    dex_gdtbase[sel].att2=attb2 | ((limit >> 16)&0xF);
    dex_gdtbase[sel].highaddr=base >> 24;
#endif
  };

void setattb(WORD sel,BYTE attb1)
  {
#ifdef __x86_64__
    (void)sel; (void)attb1;
    return;
#else
    sel=sel >> 3;
    dex_gdtbase[sel].att1=attb1;
#endif
  };

void setcallgate(DWORD sel,DWORD funcsel,void *entry,BYTE params,BYTE access)
  {
    CALLGATE *ptr=(CALLGATE*)dex_gdtbase;
    DWORD loc=(DWORD)entry;
    sel=sel>>3 ; //convert selector to an index
    ptr[sel].lowoffset=loc;
    ptr[sel].selector=funcsel;
    ptr[sel].attb1=params;
    ptr[sel].attb2=0x8c | (access << 5 );
    ptr[sel].highoffset=loc >> 16;
  };
extern void reset_gdtr();

void dex32_setbase(WORD sel,DWORD addr)
  {
#ifdef __x86_64__
    (void)sel; (void)addr;
    return;
#else
    DWORD cpuflags;
    sel=sel >> 3;
    dex_gdtbase[sel].lowaddr=addr;
    dex_gdtbase[sel].lowaddr2=addr >> 16;
    dex_gdtbase[sel].highaddr=addr >> 24;
#endif
  };



void  setinterruptvector(DWORD index,idtentry *t,unsigned char attr,
                           void (*handler)(int irq), WORD sel){
#ifdef __x86_64__
   uintptr addr = (uintptr)handler;
   /* Never install task gates on x86_64 — force interrupt/trap gate. */
   if ((attr & 0x0F) == 0x05)
      attr = (attr & 0xF0) | 0x0E;
   t[index].lowphy  = (WORD)(addr & 0xFFFF);
   t[index].selector = sel;
   t[index].ist = 0;
   t[index].attr = attr;
   t[index].midphy = (WORD)((addr >> 16) & 0xFFFF);
   t[index].highphy = (DWORD)((addr >> 32) & 0xFFFFFFFF);
   t[index].reserved = 0;
#else
   t[index].lowphy=(WORD)handler; //set the low word
   t[index].highphy=((DWORD)handler >> 16);	//set the high word
   t[index].selector=sel;
   t[index].reserved=0;
   t[index].attr=attr;
#endif
};

DWORD obtainpage()
    {
      // print("DEX 32: out of memory error\n");
      return 0;
    };



void *commit(DWORD virtualaddr,DWORD pages)
    {
     int i;
     char temp[255];
     void *ret=(void*)virtualaddr;
     DWORD flags;
     storeflags(&flags);
     stopints();
     
     #ifdef MEM_LEAK_CHECK
     printf("system committed %d pages.\n",pages);
     #endif
   
     for (i=0;i<pages;i++)
        {
          DWORD pageadr=(DWORD)mempop();
          
          //if out of physical address, call the VMM
          if (pageadr==0) pageadr=obtainpage();
          
          //out of memory error
          if (pageadr == -1) {ret = -1;break;};
          
          maplineartophysical2((DWORD*)SYS_PAGEDIR_VIR,virtualaddr,pageadr,PG_PRESENT);
          maplineartophysical2((DWORD*)SYS_KERPDIR_VIR,virtualaddr,pageadr,PG_PRESENT);
          virtualaddr+=0x1000;

        };
        
     restoreflags(flags);   
   
     return ret;
    };

void *commitb(DWORD virtualaddr,int amt,DWORD *pagecount)
   {
     int pages=(amt/4096)+1;
     char *ret=0;
     if (amt==0) pages=1;
        else
     if (amt%4096==0) pages=amt/4096;
     ret=commit(virtualaddr,pages);
     /*DEBUG*/

     *pagecount=pages;
     return ret;
   };

void *sbrk(int amt)
   {
     int pages=(amt/4096)+1;
     char *ret=0;
     
     //cannot handle negative values as of the moment
     if (amt<0) return -1;
     
     //return location of break
     if (amt==0) return knext-1;
     
     if (amt%4096==0) pages=amt/4096;
     
     ret=commit((DWORD)knext,pages);

     knext+=(pages)*4096;
     
     return (void*)ret;
   };



//defines the USER MODE function for changing the break of an
//application
void *dex32_sbrk(unsigned int amt)
   {
     DWORD pages=(amt/4096)+1;
     DWORD flags;
     char *ret=current_process->knext;
     dex32_stopints(&flags);
     if (amt==0)
        {
        dex32_restoreints(flags);
        return ((void*)current_process->knext);
        };
     if (amt%4096==0) pages=amt/4096;

     /* Commit pages into the process page directory so malloc/sbrk
        used by the in-OS compiler can grow beyond the initial heap. */
     dex32_commit((DWORD)ret, pages,
                  (DWORD*)current_process->pagedirloc, PG_USER | PG_WR);

     /* Zero the newly committed user memory before handing it out.
        The x86_64 user window is a reserved identity-mapped range, so
        dex32_commit() does not allocate/clear physical frames; without
        this, sbrk() would return stale RAM. That breaks any libc whose
        malloc relies on zeroed fresh pages (ICS-OS SDK malloc hands the
        raw sbrk() bytes to the caller), which made the in-OS TinyCC
        linker read uninitialized memory and emit a non-deterministic,
        corrupted tccnew.exe. Zeroing matches Linux brk()/mmap()
        semantics and prevents leaking stale process memory. */
     memset(ret, 0, (size_t)(pages * 4096));

     current_process->knext+=pages*4096;
     dex32_restoreints(flags);
     return (void*)ret;
   };


void dex32_copy_on_write(DWORD *directory)
{
  /*   DWORD i;
     DWORD userstart=(DWORD)userspace >> 22,
           userend  =0xC0000000 >> 22,


     for (i=userstart;i<=userend;i++)
       {


       ;};

    */
;};


//copies a pagedirectory except the user space area
//0xA0000000-0xC0000000
void dex32_copy_pagedirU(DWORD *destdir,DWORD *source)
   {
     DWORD i;
     /* Kernel identity-maps 0-16MB (image + heap + VFS nodes). User
        processes must keep that map or syscalls like fopen() page-fault
        once the kernel heap grows past 4MB. */
     DWORD kernelend = 0x01000000 >> 22,
           userend  =0xC0000000 >> 22,
           stopaddr =0xF0000000 >> 22;
     disablepaging();
    // memset(destdir,0,0x1000);
     for (i=0;i<kernelend;i++)
        destdir[i]=source[i];
     for (i=userend;i<stopaddr;i++)
        destdir[i]=source[i];
   };

int dex32_copy_pg(DWORD *destdir, DWORD *source)
   {
      DWORD i,i2;
      DWORD start = (DWORD) userspace >> 22;
      DWORD end = 0xC0000000 >> 22;
      DWORD syscallstart = 0x90000000 >> 22, syscallend=0xA0000000 >> 22;
      dex32_copy_pagedirU(destdir,source);
      for (i=start;i<end;i++)
        {
          if (source[i]&1)
            {
                  DWORD *destpagetable,*srcpagetable;
                  destdir[i]= (DWORD)mempop() | 1 | PG_USER | PG_WR;

                  destpagetable= (DWORD)destdir[i]&0xFFFFF000;
                  srcpagetable = (DWORD)source[i]&0xFFFFF000;

                  for (i2= 0 ;i2 < 1024; i2++)
                  {

                     if (srcpagetable[i2]&1)
                      {
                 		      DWORD free_mem = (DWORD)mempop();
                              //out of memory?
                              if (free_mem==-1) return -1;

                              destpagetable[i2] = free_mem | 1;

                              if (i>=syscallstart&&i<syscallend)
                                  {
                                        memcpy(destpagetable[i2]&0xFFFFF000,srcpagetable[i2]&0xFFFFF000,0x1000);
                                  }
                                    else
                                  {
                                  		memcpy(destpagetable[i2]&0xFFFFF000,srcpagetable[i2]&0xFFFFF000,0x1000);
                                        destpagetable[i2]= destpagetable[i2] | PG_WR | PG_USER;
                                  };
                      }
                        else
                     destpagetable[i2]= srcpagetable[i2];
                  };
            }
              else
            destdir[i] = source[i];
        };
   };

//copies a pagedirectory given a source and a destination
void dex32_copy_pagedir(DWORD *destdir,DWORD *source)
   {
     int i;
     for (i=0;i<(0x1000/2);i++)
        destdir[i]=source[i];
   };

//designed for  USER MODE applications
//commits a block of memory at a time in bytes
void *dex32_commitblock(DWORD virtualaddr,int amt,
    DWORD *pagecount,DWORD *pagedir,DWORD attb)
   {
     int pages=(amt/4096)+1;
     char *ret=0;
     if (amt==0) pages=1;
        else
     if (amt%4096==0) pages=amt/4096;      
     
     ret=dex32_commit(virtualaddr,pages,pagedir,attb);
     
     *pagecount=pages;
     return ret;
   };


void *dex32_reserveblock(DWORD virtualaddr,int amt,
    DWORD *pagecount,DWORD *pagedir,DWORD attb)
   {
     int pages=(amt/4096)+1;
     char *ret=0;
     if (amt==0) pages=1;
        else
     if (amt%4096==0) pages=amt/4096;
     ret=dex32_reserve(virtualaddr,pages,pagedir,attb);
     *pagecount=pages;
     return ret;
   };
//sets a virtual address location as demand paged..
//meaning, the OS commits a pge only if it has been accessed
void *dex32_reserve(DWORD virtualaddr,DWORD pages,DWORD *pagedir,DWORD attb)
   {
     int i;
     char temp[255],temp2[255];
     void *ret=(void*)virtualaddr;
     DWORD *pg;
     for (i=0;i<pages;i++)
        {
          DWORD pageadr; //holds a physical frame, if possible

          pg=(DWORD*)getvirtaddress((DWORD)pagedir);
          maplineartophysical2(pg,virtualaddr,
          0,PG_USER | PG_DEMANDLOAD | attb);

          if (current_process->accesslevel==ACCESS_SYS)
          {
          pg=(DWORD*)getvirtaddress((DWORD)pagedir1);
          maplineartophysical2(pg,virtualaddr,
          pageadr,PG_DEMANDLOAD | PG_USER | attb);
          virtualaddr+=0x1000;
          };
         };
    refreshpages();
     return ret;
    };

//designed for USER MODE applications
//commits a block of memory at a time in pages
#ifdef __x86_64__
u64 *userpd_map_page(u64 *pml4, unsigned long long vaddr, unsigned long attb);
#endif

void *dex32_commit(DWORD virtualaddr,DWORD pages,DWORD *pagedir,DWORD pattb)
    {
     int i;
     char temp[255],temp2[255];
     void *ret=(void*)virtualaddr;
     DWORD *pg;
     DWORD flags;
     
 #ifdef __x86_64__
     /* A user process with a private PML4: lazily map the committed pages
        (heap/sbrk growth) to fresh private frames. Kernel identity map or
        already-mapped regions are left alone. */
     if (pagedir && pagedir != (DWORD *)(uintptr)pagedir1) {
        int pi;
        for (pi = 0; pi < pages; pi++)
           if (!userpd_map_page((u64 *)(uintptr)pagedir,
                                (unsigned long long)(virtualaddr + (unsigned long long)pi * 0x1000),
                                (unsigned long)pattb))
              break;
     }
     (void)i;
     (void)temp;
     (void)temp2;
     (void)pg;
     (void)flags;
     return ret;
 #else
     storeflags(&flags);
     stopints();

     /*DEBUG*/

     #ifdef MEM_LEAK_CHECK
     printf("user committed %d pages.\n",pages);
     #endif
     for (i=0;i<pages;i++)
        {
          DWORD pageadr; //holds a physical frame, if possible
        //if page is set to demand paged, we do not commit
        //a physical frame yet
          pageadr=(DWORD)mempop();

          if (pageadr==0) //we're out of physical frames, find a way to get one
          pageadr=obtainpage(); //call the VMM to get a page


          pg=(DWORD*)getvirtaddress((DWORD)pagedir);


          maplineartophysical2(pg,virtualaddr, pageadr,PG_PRESENT | /*PG_USER |*/ pattb);
              //if this is called from the KERNEL map it to kernel memory too
              if (current_process->accesslevel==ACCESS_SYS)
               {
                   pg=(DWORD*)getvirtaddress((DWORD)pagedir1);
        
                   maplineartophysical2(pg,virtualaddr,
                   pageadr,PG_PRESENT | /*PG_USER |*/ pattb);
               };
           
          virtualaddr+=0x1000;
         };
         
     refreshpages();
     restoreflags(flags);
     
     return ret;
#endif
    };

void dex32copyblock(DWORD vdest,DWORD vsource,DWORD pages,DWORD *pagedir)
   {
     int i;
     for (i=0;i<pages;i++)
          {
           DWORD pdest=getphys(vdest,pagedir);
           DWORD pg=(DWORD*)getvirtaddress((DWORD)pdest);
           memcpy((void*)pg,(void*)vsource,0x1000);
           vdest+=0x1000;
           vsource+=0x1000;
          };

   };


void dex32_restore_identity_map(void)
{
#ifdef __x86_64__
   extern u64 boot_pd0[], boot_pd1[], boot_pd2[], boot_pd3[];
   extern u64 boot_pml4[];
   u64 *pds[4];
   u64 phys = 0;
   int pd, i;

   /* User processes currently run in kernel CS and can corrupt the boot
      2MiB identity map (e.g. clear R/W). Reinstall P|RW|PS before each exec. */
   pds[0] = boot_pd0;
   pds[1] = boot_pd1;
   pds[2] = boot_pd2;
   pds[3] = boot_pd3;
   for (pd = 0; pd < 4; pd++) {
      for (i = 0; i < 512; i++) {
         pds[pd][i] = phys | 0x83ULL; /* present | writable | page-size */
         phys += 0x200000ULL;
      }
   }
   __asm__ volatile ("mov %0, %%cr3" :: "r"((unsigned long)(uintptr)boot_pml4) : "memory");
#endif
}

/* ------------------------------------------------------------------
 * Per-process user page directories (x86_64)
 *
 * On x86_64 every user ELF previously ran with pagedir1 (the shared
 * kernel identity map), so ALL user processes shared the SAME physical
 * stack/heap/ELF/syscall-stack memory. Concurrent user processes (e.g.
 * in-OS tcc compiling tccnew.exe) clobbered each other's C-stack frames
 * in the shared 0xE000000 stack window; on resume the restored frame had
 * a corrupted SS and the CPU raised #13 (GPF) with the live RIP inside
 * the process's own BSS and RAX == CR3.
 *
 * userpd_create() builds a PRIVATE PML4 for one user process: it shares
 * the kernel's four 1GiB superpage page tables (boot_pd0..3) so the
 * kernel image, its BSS and the frame pool are reachable, but it
 * privatizes the user-window 2MiB blocks (the ELF image at 0x400000+,
 * the syscall stack at 0x9000000, the heap at 0xA000000+, the stack at
 * 0xE000000) with per-process 4KiB frames drawn from a DEDICATED pool
 * (below). Those frames are zeroed at allocation, matching brk()/mmap()
 * semantics. context.S already switches CR3 per task (ctx.cr3), and
 * createprocess() sets ctx.cr3 = pagedir, so the private PML4 is simply
 * installed into the new task's context.
 *
 * The frames come from a dedicated pool, NOT the shared mempop() freelist:
 * getphys() is identity on x86_64 and freelinearloc() would mempush() the
 * identity addresses back into the shared pool, which overlaps the kernel
 * heap (knext grows from 0x03000000 inside the [0x300000,0x8000000)
 * frame range) and corrupts the kernel. The pool below is a disjoint
 * physical range, so it never contends with the kernel allocator.
 * ------------------------------------------------------------------ */

/* Physical range reserved for per-process user page tables + PTE frames.
   It sits BELOW the identity-mapped user window [0x08000000,0x10000000)
   (syscallstack 0x09000000 / userheap 0x0A000000 / userstackloc 0x0E000000)
   and is carved out of the shared mempop() freelist (seeded
   [0x00400000,0x08000000)), with mem_detectmemory() told to skip it so the
   kernel allocator never double-allocates a pool frame.

   The pool MUST NOT overlap the identity-mapped user window: the kernel runs
   under pagedir1 (identity), so its writes to the user heap/stack (loader
   memsets, sbrk) hit those PHYSICAL addresses. If the pool lived there, the
   kernel's identity writes would clobber the process's own private frames
   (its code/stack) -> wild RIP + GPF. Placing it below 0x08000000 keeps the
   process's private frames disjoint from everything the kernel writes by
   identity. 32MiB = 8192 frames covers a full 8MiB heap + 1MiB stack + ELF
   image + syscall stack + page tables for a user process. */

/* Dedicated pool allocator.
 *
 * The free state is a BITMAP in .bss (1024 bytes for 8192 frames), NOT
 * metadata stored inside the frames. The previous design kept a next-free
 * pointer in each frame's first 8 bytes; because the same frames also hold
 * the live PML4/PDPT/PD page-table pages and the process's code/stack/heap
 * pages, any stray kernel write into the pool range (identity-mapped) could
 * clobber the allocator's own bookkeeping and silently hand out a wrong or
 * already-used frame -> wild-RIP GPF. A bitmap keeps the allocator state
 * disjoint from the allocated memory, the standard approach for physical
 * frame allocation. upop() always memsets/memcpy's a frame before use, so
 * stale frame contents are irrelevant. */
static u8 up_bitmap[USERPD_MAXFRAMES / 8];   /* bit=1 -> allocated */
static int up_inited = 0;

static int up_valid_frame(unsigned long long p)
{
   return (p >= UPD_POOL_BASE && p < UPD_POOL_TOP);
}

static void userpd_init_frames(void)
{
   if (up_inited) return;
   memset(up_bitmap, 0, sizeof(up_bitmap));   /* all free */
   up_inited = 1;
}

/* Pop a physical frame from the dedicated pool. Returns 0 when empty.
   Scans highest-first to match the previous ordering (deterministic CR3s). */
static DWORD *upop(void)
{
   int i;
   userpd_init_frames();
   for (i = USERPD_MAXFRAMES - 1; i >= 0; i--) {
      int byte = i >> 3, bit = i & 7;
      if (!(up_bitmap[byte] & (1u << bit))) {
         up_bitmap[byte] |= (1u << bit);
         return (DWORD *)(uintptr)(UPD_POOL_BASE + (unsigned long long)i * 0x1000ULL);
      }
   }
   return 0;   /* pool empty */
}

/* Return a physical frame to the dedicated pool (only if it is ours). */
static void upush(DWORD *fr)
{
   unsigned long long p = (unsigned long long)(uintptr)fr;
   if (!up_valid_frame(p)) return;
   {
      unsigned long long idx = (p - UPD_POOL_BASE) >> 12;
      up_bitmap[idx >> 3] &= ~(1u << (idx & 7));
   }
}

/* Map one 4KiB page (vaddr) in a user PML4 to a fresh, zeroed private
   frame. attb: e.g. PG_USER|PG_WR. Returns the physical frame (for the
   kernel to copy/zero) or 0 on allocation failure.

   Walks the real 4-level hierarchy that startup.S builds:
       PML4[0] -> PDPT ; PDPT[block] -> PD(2MiB pages) ; PD[page] -> 4KiB.
   The user window lives in the first GiB, so PML4 index is 0 and the
   relevant PD is PDPT[0] (our private copy of boot_pd0). To map a 4KiB
   page inside a 2MiB block we replace that block's 2MiB page entry (PS)
   with a private 4KiB PTE table on first touch, then map the page. */
u64 *userpd_map_page(u64 *pml4, unsigned long long vaddr, unsigned long attb)
{
   int pmi = (int)((vaddr >> 39) & 0x1FF);   /* PML4 index   (GiB)  */
   int pi  = (int)((vaddr >> 30) & 0x1FF);   /* PDPT index   (1GiB) */
   int bi  = (int)((vaddr >> 21) & 0x1FF);   /* 2MiB block   within PD */
   int gi  = (int)((vaddr >> 12) & 0x1FF);   /* 4KiB page    within block */
   u64 pe, de;
   u64 *pdpt, *pd, *pte, *fr;

   pe = pml4[pmi];
   if (!(pe & 1) || (pe & 0x80)) return 0;   /* PML4[pmi] bad */
   pdpt = (u64 *)(pe & 0x000FFFFFFFFF000ULL);

   de = pdpt[pi];
   if (!(de & 1) || (de & 0x80)) return 0;   /* PDPT[pi] bad */
   pd = (u64 *)(de & 0x000FFFFFFFFF000ULL);

   /* Resolve the 2MiB block: an absent entry or a 2MiB identity page (PS)
      becomes a fresh private 4KiB PTE table; an existing 4KiB PTE table is
      reused. (upop() returns a frame pointer or NULL, never a PTE.) */
   {
      u64 be = pd[bi];
      if (be & 1 && !(be & 0x80)) {
         /* already a private 4KiB PTE table: reuse */
      } else {
         u64 *t = upop();
         if (!t) return 0;   /* pool empty (PTE table) */
         memset(t, 0, 0x1000);
         pd[bi] = (u64)(uintptr)t | 0x03ULL;
      }
   }
   pte = (u64 *)(pd[bi] & 0x000FFFFFFFFF000ULL);

   {
      u64 e = pte[gi];
      if (e & 1) return (u64 *)(e & 0x000FFFFFFFFF000ULL); /* already mapped */
      fr = upop();
      if (!fr) return 0;   /* pool empty (4KiB page) */
      memset(fr, 0, 0x1000);
      /* A mapped leaf page MUST be present: force PG_PRESENT. Callers pass
         only the additional attributes (PG_WR|PG_USER|...), so OR in the
         Present bit here or the page would be invisible to the CPU. */
      pte[gi] = (u64)(uintptr)fr | (attb | PG_PRESENT);
      return fr;
   }
}

/* Allocate a private PML4 mirroring the kernel's hierarchy (startup.S):
   PML4[0]->PDPT ; PDPT[0]->PD0 (private copy) ; PDPT[1..3]->boot_pd1..3.
   The private PD0 is a copy of boot_pd0 (512 x 2MiB identity pages) so the
   kernel image/heap/BSS stay reachable; user-window 2MiB blocks are later
   replaced by 4KiB PTE tables by userpd_map_page(). */
u64 *userpd_create(void)
{
   extern u64 boot_pd0[], boot_pd1[], boot_pd2[], boot_pd3[];
   u64 *pml4, *pdpt, *pd0;
   pml4 = upop();
   if (!pml4) return 0;
   memset(pml4, 0, 0x1000);
   pdpt = upop();
   if (!pdpt) { upush(pml4); return 0; }
   memset(pdpt, 0, 0x1000);
   pd0 = upop();
   if (!pd0) { upush(pdpt); upush(pml4); return 0; }
   memcpy(pd0, boot_pd0, 0x1000);       /* copy 512 x 2MiB identity entries */
   pdpt[0] = (u64)(uintptr)pd0  | 0x03ULL;
   pdpt[1] = (u64)(uintptr)boot_pd1 | 0x03ULL;
   pdpt[2] = (u64)(uintptr)boot_pd2 | 0x03ULL;
   pdpt[3] = (u64)(uintptr)boot_pd3 | 0x03ULL;
   pml4[0] = (u64)(uintptr)pdpt | 0x03ULL;
   return pml4;
}

/* Map the whole user window [base, base+size) with private frames. */
void userpd_map_region(u64 *pml4, unsigned long long base, unsigned long long size,
                       unsigned long attb)
{
   unsigned long long va;
   for (va = base & ~0xFFFULL; va < base + size; va += 0x1000)
      if (!userpd_map_page(pml4, va, attb))
         break;
}

/* Free a private PML4: walk PML4->PDPT->PD/PTE and return every private
   pool frame (PTE tables + 4KiB pages + PD + PDPT + PML4). Shared 2MiB
   identity pages (PS set) and the shared boot_pd1..3 are left alone. */
void userpd_free(u64 *pml4)
{
   if (!pml4) return;
   {
      u64 pe = pml4[0];
      if ((pe & 1) && !(pe & 0x80)) {
         u64 *pdpt = (u64 *)(pe & 0x000FFFFFFFFF000ULL);
         if (up_valid_frame((unsigned long long)(uintptr)pdpt)) {
            int i;
            for (i = 0; i < 512; i++) {
               u64 de = pdpt[i];
               if (!(de & 1) || (de & 0x80)) continue;   /* absent / 2MiB page */
               u64 *tbl = (u64 *)(de & 0x000FFFFFFFFF000ULL);
               if (!up_valid_frame((unsigned long long)(uintptr)tbl)) continue;
               /* tbl is our private PD (copy of boot_pd0) or a PTE table.
                  Free every private 4KiB frame it references. */
               {
                  int j;
                  for (j = 0; j < 512; j++) {
                     u64 je = tbl[j];
                     if (!(je & 1) || (je & 0x80)) continue;
                     u64 *inner = (u64 *)(je & 0x000FFFFFFFFF000ULL);
                     if (up_valid_frame((unsigned long long)(uintptr)inner)) {
                        int k;
                        for (k = 0; k < 512; k++)
                           if (inner[k] & 1)
                              upush((u64 *)(inner[k] & 0x000FFFFFFFFF000ULL));
                        upush(inner);
                     }
                  }
               }
               upush(tbl);
            }
            upush(pdpt);
         }
      }
   }
   upush((u64 *)pml4);
}

void mem_init()
{
#ifdef __x86_64__
    extern u64 boot_pml4[];
    /* Long mode already has an identity map via boot_pml4; keep CR3. */
    pagedir1 = (DWORD *)(uintptr)boot_pml4;
    (void)pagedir1;
#else
    DWORD i;
    char temp[255];
    pagedir1=mempop(); //obtain the first pagedirectory

    clearpagetable(pagedir1);
    /* Identity-map low memory as writable. GDT/IDT, the free-page stack
       (0x200000) and the kernel image all live here; a read-only map
       page-faults on CPUs that honor CR0.WP. */
    for (i=0;i<0xB8000;i+=0x1000)
        maplineartophysical((DWORD*)pagedir1,i,i        /*,stackbase*/,1 | PG_WR );
    for (i=0xb8000;i<0x100000;i+=0x1000)
        maplineartophysical((DWORD*)pagedir1,i,i        /*,stackbase*/,1 | PG_USER | PG_WR );
    for (i=0x100000;i<0x1000000;i+=0x1000)
        maplineartophysical((DWORD*)pagedir1,i,i        /*,stackbase*/,1 | PG_WR );

    maplineartophysical((DWORD*)pagedir1,(DWORD)SYS_PAGEDIR_VIR,(DWORD)pagedir1    /*,stackbase*/,1);
    maplineartophysical((DWORD*)pagedir1,(DWORD)SYS_PAGEDIR2_VIR,
    (DWORD)pagedir1[SYS_PAGEDIR_VIR >> 22]&0xFFFFF000,1);
    maplineartophysical((DWORD*)pagedir1,(DWORD)SYS_PAGEDIR3_VIR,
    (DWORD)pagedir1[SYS_PAGEDIR_VIR >> 22]&0xFFFFF000,1);
    maplineartophysical((DWORD*)pagedir1,(DWORD)SYS_KERPDIR_VIR,(DWORD)pagedir1    /*,stackbase*/,1);

    setpagedir(pagedir1);
    enablepaging();
#endif
};

/*This function registers the memory amanger to the device manager so that it's
  interface could be used by other device drivers*/
void mem_register()
{
devmgr_mem memory_manager;

//Set up the usual things the device manager needs
memory_manager.hdr.size = sizeof(devmgr_mem);
strcpy(memory_manager.hdr.name,"mem_mgr");
strcpy(memory_manager.hdr.description,"DEX low-level memory manager");
memory_manager.hdr.type = DEVMGR_MEM;

/* Fill up the service functions that the device manager provices, this
   functions will be visible to other modules that use the mem_mgr interface*/
   
memory_manager.sbrk = sbrk;
memory_manager.mem_map = maplineartophysical2;
memory_manager.commit = commit;
memory_manager.freemultiple = freemultiple;

/*register myself to the device manager*/
devmgr_register( (devmgr_generic*) &memory_manager );

};

