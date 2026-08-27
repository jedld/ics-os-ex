//**************************************************************************
//DEX32 Disk drive scheduler
//April 3, 2003 by Joseph Emmanuel Dayo
//
//P0: block I/O runs in the caller without holding IOrequest_busy across the
//device transfer.  A per-device lock serializes ATA/UHCI (not SMP-safe to
//issue two PIO commands at once).  disk_mgr sleeps between flush passes
//instead of spinning; iomgr_request_flush() wakes it.
//**************************************************************************

#include "../devmgr/dex32_devmgr.h"
#include "iosched.h"
#include "blkcache.h"
#include "../stdlib/time.h"
#include "../process/process.h"

extern void *malloc(unsigned int);
extern void *memset(void *s, int c, unsigned int n);
extern void printf(const char *fmt, ...);
extern char *strcpy(char *d, const char *s);
extern int shouldflush(void);

int IOmgr_pause=0;
int IOrequest_time = 0;
int flush_time=10;
int flush_counter=0;
int forceflush=0;
IOrequest *IOjob;
IOrequest *cur;
int io_flushid = 0;
int flushing=0,flushok=1;

sync_sharedvar IOrequest_busy;
static sync_sharedvar io_devlock[MAXDEVICES];
static DWORD iomgr_diskmgr_pid;

extern PCB386 *ps_findprocess(DWORD processid);

static void iomgr_lockdev(int deviceid)
{
   if (deviceid >= 0 && deviceid < MAXDEVICES)
      sync_entercrit(&io_devlock[deviceid]);
}

static void iomgr_unlockdev(int deviceid)
{
   if (deviceid >= 0 && deviceid < MAXDEVICES)
      sync_leavecrit(&io_devlock[deviceid]);
}

void iomgr_register_diskmgr(DWORD pid)
{
   iomgr_diskmgr_pid = pid;
}

void iomgr_request_flush(void)
{
   PCB386 *p;
   forceflush = 1;
   if (!iomgr_diskmgr_pid)
      return;
   p = ps_findprocess(iomgr_diskmgr_pid);
   if (p && p != (PCB386 *)-1)
      p->waiting = 0;
}

DWORD iomgr_init()
 {
   devmgr_iomgr iomgr;
   int i;
   IOjob=0;
   cur=0;
   iomgr_diskmgr_pid = 0;
   memset(&IOrequest_busy,0,sizeof(sync_sharedvar));
   memset(io_devlock, 0, sizeof(io_devlock));

   blkcache_init();

   strcpy(iomgr.hdr.name,"default_iomgr");
   strcpy(iomgr.hdr.description,"DEX default I/O scheduler and manager");
   iomgr.hdr.type = DEVMGR_IOMGR;
   iomgr.hdr.size = sizeof(devmgr_iomgr);
   iomgr.init = iomgr_init;
   iomgr.complete = dex32_IOcomplete;
   iomgr.close = dex32_closeIO;
   iomgr.request = dex32_requestIO;
   devmgr_register((devmgr_generic*)&iomgr);
   return 1;
 };

DWORD iomgr_flushmgr()
 {
 int ret;


 ret = devmgr_flushblocks();
 if (ret==-1)
    printf("Error writing to device %d. Data might be lost.\n",ret);

 };

static u64 iomgr_execjob(IOrequest *ptr);
DWORD iomgr_diskmgr()
{
   IOrequest *ptr;
   u64 lastjob=0;
   flush_counter=time();
   do
    {

      while (IOmgr_pause)
         sleep(1);

      ptr=IOmgr_obtainjob(0, lastjob);
      if (ptr==0){
		        if ( (flush_counter - time() > flush_time) ||
                   (time() - flush_counter > flush_time) || forceflush)
                   {
                     forceflush=0;

                     if (shouldflush()) iomgr_flushmgr();
                     flush_counter=time();
                    };
          /* Yield a scheduler pass, then halt until the next IRQ. */
          sleep(1);
          cpu_idle();
          continue;};


      if (ptr!=0)
      {
          do {
             lastjob=iomgr_execjob(ptr);
             ptr= IOmgr_obtainjob(0, lastjob);

           }

           while ( ptr!=0);
      };

     } while (1)

 ;};

/* Device I/O only. Caller must hold the per-device lock, not IOrequest_busy. */
static u64 iomgr_execjob(IOrequest *ptr)
{
   devmgr_block_desc *myblock;

   myblock = (devmgr_block_desc*)devmgr_devlist[ptr->deviceid];

   devmgr_setcontext(ptr->deviceid);

   if (myblock->hdr.type!=DEVMGR_BLOCK)
   {
      printf("IO ERROR: Device %d is not a block device!\n", ptr->deviceid);
      ptr->status=IO_ERROR;
      return ptr->lba;
   };

   if (ptr->type==IO_READ)
   {
      if (myblock->read_block(ptr->lba,ptr->buf,ptr->num_of_blocks) > 0)
      {
         ptr->status=IO_COMPLETE;
         if ((myblock->hdr.name[0] != 'c' || myblock->hdr.name[1] != 'd') &&
             myblock->putcache == 0) {
            blkcache_put(ptr->deviceid, ptr->lba,
                         ptr->num_of_blocks, ptr->buf);
         }
      }
      else
         ptr->status=IO_ERROR;
   }
   else if (ptr->type==IO_WRITE)
   {
      if (myblock->write_block==0)
      {
         printf("IO ERROR: Device %d does not support writes!\n", ptr->deviceid);
         ptr->status=IO_ERROR;
      }
      else if (myblock->write_block(ptr->lba,ptr->buf,ptr->num_of_blocks))
         ptr->status=IO_COMPLETE;
      else
         ptr->status=IO_ERROR;
   }
   else
      ptr->status=IO_ERROR;

   return ptr->lba;
};

IOrequest *IOmgr_obtainjob(int deviceid, u64 near_lba)
 {
  IOrequest *ptr,*handlecand;
  u64 mindist=~(u64)0;

  sync_entercrit(&IOrequest_busy);

  if (IOjob==0) {
          sync_leavecrit(&IOrequest_busy);
          return 0;
            };

  ptr=IOjob;
  handlecand = IOjob;

  while (ptr!=0)
   {
      u64 dist;

      if ( near_lba > ptr->lba)
           dist=near_lba-ptr->lba;
      else
           dist=ptr->lba-near_lba;

      if (dist<mindist)
         {
          handlecand=ptr;
          mindist=dist;
         };

     ptr=ptr->next;
   ;};

   ptr=handlecand;

  if (ptr == IOjob)
      IOjob=ptr->next;

  if (ptr->prev!=0)
      ptr->prev->next=ptr->next;

  if (ptr->next!=0)
      ptr->next->prev=ptr->prev;

  sync_leavecrit(&IOrequest_busy);
  return ptr;
;};

int dex32_IOcomplete(DWORD handle)
  {
  int retval;
  IOrequest *ptr;

  ptr=(IOrequest*)handle;

  if (ptr->status==IO_COMPLETE) retval=1;
     else
  if (ptr->status==IO_ERROR) retval=-1;
     else
  if (ptr->status==IO_PENDING) retval=0;
  else
     retval = -1;

  return retval;

  ;};

void dex32_closeIO(DWORD handle)
  {
  IOrequest *ptr;

  ptr=(IOrequest*)handle;
  free(ptr);
  ;};

static IOrequest *iomgr_alloc_done(int type, u64 block, DWORD numblocks, void *buf, int status)
{
   IOrequest *ptr=(IOrequest*)malloc(sizeof(IOrequest));
   if (!ptr) return 0;
   memset(ptr, 0, sizeof(IOrequest));
   ptr->rID=(DWORD)(uintptr)ptr;
   ptr->type = type;
   ptr->lba = block;
   ptr->num_of_blocks = numblocks;
   ptr->status = status;
   ptr->buf=buf;
   return ptr;
}

DWORD dex32_requestIO(int deviceid,int type,u64 block,DWORD numblocks, void *buf)
{
     IOrequest *ptr;
     devmgr_block_desc *myblock = (devmgr_block_desc*) devmgr_getdevice(deviceid);

     if (!myblock)
        return 0;

     if (myblock->getcache!=0)
     if (type==IO_READ&&myblock->getcache(buf,(DWORD)block,numblocks))
      {
        ptr=iomgr_alloc_done(type, block, numblocks, buf, IO_COMPLETE);
        return ptr ? ptr->rID : 0;
      };

     if (type==IO_READ && myblock->getcache==0 &&
         blkcache_get(deviceid, block, numblocks, buf))
      {
        ptr=iomgr_alloc_done(type, block, numblocks, buf, IO_COMPLETE);
        return ptr ? ptr->rID : 0;
      };

      if (myblock->putcache!=0)
      if (type==IO_WRITE&&myblock->putcache(buf,(DWORD)block,numblocks))
      {
        ptr=iomgr_alloc_done(type, block, numblocks, buf, IO_COMPLETE);
        return ptr ? ptr->rID : 0;
      };

     ptr=(IOrequest*)malloc(sizeof(IOrequest));
     if (!ptr) return 0;
      memset(ptr, 0, sizeof(IOrequest));
      ptr->deviceid = deviceid;
      ptr->rID=(DWORD)(uintptr)ptr;
      ptr->type = type;
      ptr->lba = block;
      ptr->status = IO_PENDING;
      ptr->buf=buf;
      ptr->time=IOrequest_time++;
      ptr->num_of_blocks = numblocks;

      /* Per-device lock only.  Do not hold IOrequest_busy or disable
         task switching across the transfer — that serialized the whole
         system on ATA PIO. */
      iomgr_lockdev(deviceid);
      iomgr_execjob(ptr);
      iomgr_unlockdev(deviceid);
      return ptr->rID;
;}
