//**************************************************************************
//DEX32 Disk drive scheduler
//April 3, 2003 by Joseph Emmanuel Dayo
//This currently implements a very simple first-come-first-served IO queue
//**************************************************************************

#include "../devmgr/dex32_devmgr.h"
#include "iosched.h"
#include "blkcache.h"
#include "../stdlib/time.h"
#include "../process/process.h"

//The registers that the io scheduler uses
int IOmgr_pause=0;
int IOrequest_time = 0;
int flush_time=10;  /* Time in seconds before writes are flushed to the disk. Only
                       for devices that have caches*/
int flush_counter=0;
int forceflush=0;
IOrequest *IOjob;
IOrequest *cur;
int io_flushid = 0;
int flushing=0,flushok=1;

sync_sharedvar IOrequest_busy;

//initializes all the data structures needed in this module
DWORD iomgr_init()
 {
   devmgr_iomgr iomgr;
   int i;
   IOjob=0;
   cur=0;
   memset(&IOrequest_busy,0,sizeof(sync_sharedvar));
   
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
 };

DWORD iomgr_flushmgr()
 {
 int ret;


 ret = devmgr_flushblocks();
 if (ret==-1)
    printf("Error writing to device %d. Data might be lost.\n",ret);

 };

//the disk manager is the worker for the blocking work-queue.  Normal
//requests are executed synchronously by dex32_requestIO() (in the
//caller's context); this thread is a safety net that drains any
//request left on the queue and periodically flushes pending writes.
//
//Why synchronous execution:  disk_mgr is a priority-0 kernel thread
//(createkthread) while user processes get priority 1 (createprocess).
//A user process that blocked in open()/read() waited on
//dex32_IOcomplete() in a taskswitch() loop, but the priority scheduler
//kept re-selecting the (runnable) user process over disk_mgr, so the
//queued CD/HD read never ran -> deadlock.  Raising the worker's
//priority is not an option: it busy-polls, so a higher priority would
//starve the user/boot path.  Running the request inline in the
//caller's context removes the dependency on the worker's scheduling
//entirely.
static DWORD iomgr_execjob(IOrequest *ptr);
DWORD iomgr_diskmgr()
{
   IOrequest *ptr;
   DWORD lastjob=0;
   flush_counter=time();
   do
    {

      while (IOmgr_pause);
      ptr=IOmgr_obtainjob(0,0,lastjob);
      if (ptr==0){
		//Commit writes to the disk if a specified time interval has
		//been met.
		        if ( (flush_counter - time() > flush_time) ||
                   (time() - flush_counter > flush_time) || forceflush)
                   {
                     forceflush=0;

                     if (shouldflush()) iomgr_flushmgr();
                     flush_counter=time();
                    };
          cpu_idle();
          continue;};

      
      if (ptr!=0)
      {
          sync_entercrit(&IOrequest_busy);   
          /*Turn of task switching to improve performance*/
          disable_taskswitching();
          
          do {
             lastjob=iomgr_execjob(ptr);
             ptr= IOmgr_obtainjob(0,0,lastjob);

           }
           
           while ( ptr!=0);    

           enable_taskswitching(); /*Turn on task switiching*/
           sync_leavecrit(&IOrequest_busy);
      };
      
     } while (1);

 ;};

/*Execute a single (dequeued) block request synchronously in the
 *current context.  Marks the request complete/error and returns its
 *lowblock for proximity-ordered picking.  Used by iomgr_diskmgr (queue
 *drain) and by dex32_requestIO (synchronous fast path).*/
static DWORD iomgr_execjob(IOrequest *ptr)
{
   devmgr_block_desc *myblock;

   myblock = (devmgr_block_desc*)devmgr_devlist[ptr->deviceid];

   devmgr_setcontext(ptr->deviceid);

   if (myblock->hdr.type!=DEVMGR_BLOCK)
   {
      printf("IO ERROR: Device %d is not a block device!\n", ptr->deviceid);
      ptr->status=IO_ERROR;
      return ptr->lowblock;
   };

   if (ptr->type==IO_READ)
   {
      /* Drivers return 1 on success; historically some returned -1
         on error, which is truthy - treat only >0 as success. */
      if (myblock->read_block(ptr->lowblock,ptr->buf,ptr->num_of_blocks) > 0)
      {
         ptr->status=IO_COMPLETE;
         if ((myblock->hdr.name[0] != 'c' || myblock->hdr.name[1] != 'd') &&
             myblock->putcache == 0) {
            blkcache_put(ptr->deviceid, ptr->lowblock,
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
      else if (myblock->write_block(ptr->lowblock,ptr->buf,ptr->num_of_blocks))
         ptr->status=IO_COMPLETE;
      else
         ptr->status=IO_ERROR;
   }
   else
      ptr->status=IO_ERROR;

   return ptr->lowblock;
};

//dequeue a request
IOrequest *IOmgr_obtainjob(int deviceid,
    DWORD lblockhigh,DWORD lblocklow /*for optimization*/)
 {
  IOrequest *ptr,*tmp,*handlecand;
  DWORD mindist=0xFFFFFFFF;

  //Wait until the IO manager is ready
  sync_entercrit(&IOrequest_busy);
  
  if (IOjob==0) {
          sync_leavecrit(&IOrequest_busy);
          return 0;
            };
            
  ptr=IOjob;
  handlecand = IOjob;
  
  while (ptr!=0)
   {
      DWORD dist;
      
      if ( lblocklow > ptr->lowblock) 
           dist=lblocklow-ptr->lowblock;
      else
           dist=ptr->lowblock-lblocklow;
           
      if (dist<mindist)
         {
          handlecand=ptr;
          mindist=dist;
         };
         
     ptr=ptr->next;
   ;};

   ptr=handlecand;

  //remove the JOB from the queue and return
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

  //wait until the I/O manager is ready
  sync_entercrit(&IOrequest_busy);
  
 
  
  ptr=(IOrequest*)handle;

  if (ptr->status==IO_COMPLETE) retval=1;
     else
  if (ptr->status==IO_ERROR) retval=-1;
     else
  if (ptr->status==IO_PENDING) retval=0;
//     else //ptr->status was given an unknown value, this is impossible
          //unless a process overwrites the IOrequest data structure
//  printf("iomgr() data structure protection error\n");
  sync_leavecrit(&IOrequest_busy);
  
  return retval;

  ;};

void dex32_closeIO(DWORD handle)
  {
  IOrequest *ptr;

  //wait until the I/O manager is ready
  sync_entercrit(&IOrequest_busy);
  
  ptr=(IOrequest*)handle;
  free(ptr);
  
  sync_leavecrit(&IOrequest_busy);
  ;};

DWORD dex32_requestIO(int deviceid,int type,DWORD block,DWORD numblocks, void *buf)
{
     IOrequest *ptr;
     devmgr_block_desc *myblock = (devmgr_block_desc*) devmgr_getdevice(deviceid);
     DWORD flags;
     //wait until the I/O manager is ready
     sync_entercrit(&IOrequest_busy);
     storeflags(&flags);
     stopints();
     #ifdef DEBUG_IOREADWRITE2
     printf("R(");
     #endif
     if (myblock->getcache!=0)
     if (type==IO_READ&&myblock->getcache(buf,block,numblocks))            
      {
        ptr=(IOrequest*)malloc(sizeof(IOrequest));
        ptr->rID=(DWORD)ptr;
        ptr->type = type;
        ptr->lowblock = block;
        ptr->num_of_blocks = numblocks;
        ptr->status = IO_COMPLETE;
        ptr->buf=buf;
       

        #ifdef DEBUG_IOREADWRITE2
        printf(")r\n");
        #endif
        restoreflags(flags);
        sync_leavecrit(&IOrequest_busy);
        return (DWORD)ptr->rID;
      };

     /* Device-agnostic cache for drivers that do not register getcache. */
     if (type==IO_READ && myblock->getcache==0 &&
         blkcache_get(deviceid, block, numblocks, buf))
      {
        ptr=(IOrequest*)malloc(sizeof(IOrequest));
        ptr->rID=(DWORD)ptr;
        ptr->type = type;
        ptr->lowblock = block;
        ptr->num_of_blocks = numblocks;
        ptr->status = IO_COMPLETE;
        ptr->buf=buf;
        restoreflags(flags);
        sync_leavecrit(&IOrequest_busy);
        return (DWORD)ptr->rID;
      };
      
      if (myblock->putcache!=0)
      if (type==IO_WRITE&&myblock->putcache(buf,block,numblocks))
      {
        ptr=(IOrequest*)malloc(sizeof(IOrequest));
        ptr->rID=(DWORD)ptr;
        ptr->type = type;
        ptr->lowblock = block;
        ptr->status = IO_COMPLETE;
        ptr->buf=buf;
        ptr->num_of_blocks = numblocks;

        
        #ifdef DEBUG_IOREADWRITE2
        printf(")r\n");
        #endif
        restoreflags(flags);
        sync_leavecrit(&IOrequest_busy);
        return (DWORD)ptr->rID;
      };
      
     //queue the request
     ptr=(IOrequest*)malloc(sizeof(IOrequest));
     if (IOjob==0) 
     {
      IOjob=ptr;
      ptr->next=0;
      ptr->prev=0;
     }
      else
     {
      ptr->next=IOjob;
      ptr->prev=0;
      IOjob->prev=ptr;
      IOjob=ptr;
     };
     
      ptr->deviceid = deviceid;    
      ptr->rID=(DWORD)ptr;
      ptr->type = type;
      ptr->lowblock = block;
      ptr->status = IO_PENDING;
      ptr->buf=buf;
      ptr->time=IOrequest_time++;
      ptr->num_of_blocks = numblocks;
      #ifdef DEBUG_IOREADWRITE2
      printf(")r\n");
      #endif
      restoreflags(flags);      
      sync_leavecrit(&IOrequest_busy);

      /* Execute the request synchronously in the caller's
         context.  disk_mgr is a priority-0 kernel thread and
         user processes get priority 1, so a user process
         blocked on dex32_IOcomplete() would never be
         descheduled in favor of disk_mgr and the queued
         read would never complete (starvation deadlock).
         Running the read/write inline makes the request
         complete before dex32_requestIO() returns; the
         worker thread only needs to drain stragglers.
         Cache-hit fast paths above already completed.
         A read of a cache-miss block executes here; the
         result is still cached by iomgr_execjob so a
         second request for the same block is served from
         the cache. */
      { DWORD fl2; storeflags(&fl2); stopints();
        sync_entercrit(&IOrequest_busy);
        disable_taskswitching();
        IOmgr_obtainjob(0,0,0);
        iomgr_execjob(ptr);
        enable_taskswitching();
        sync_leavecrit(&IOrequest_busy);
        restoreflags(fl2); }
      return (DWORD)ptr->rID;
;};

