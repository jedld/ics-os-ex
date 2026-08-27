//**************************************************************************
//DEX32 Disk drive scheduler
//April 3, 2003 by Joseph Emmanuel Dayo
//P2: bio + one blk-mq hctx per device, 64-bit LBA, no global lock across the transfer.
//**************************************************************************


#ifndef _IOSCHED_H
#define _IOSCHED_H

//IO request types
#define IO_NONE 0
#define IO_READ 1
#define IO_WRITE 2

//IO request status
#define IO_PENDING 0
#define IO_COMPLETE 1
#define IO_ERROR 2

#include "../dextypes.h"
#include "../types.h"
#include "../process/sync.h"

typedef struct _IOrequest
 {
 DWORD rID;
 int type;
 int deviceid;
 int status;
 DWORD time;
 void *buf;
 u64  lba;
 DWORD num_of_blocks;
 struct _IOrequest *next,*prev;
 } IOrequest;

extern int IOmgr_pause, IOrequest_time, flush_time ;
extern int flush_counter, forceflush;
extern IOrequest *IOjob;
extern IOrequest *cur;
extern int io_flushid, flushing, flushok;
extern sync_sharedvar IOrequest_busy;


DWORD iomgr_init();
DWORD iomgr_flushmgr();
DWORD iomgr_diskmgr();
void  iomgr_register_diskmgr(DWORD pid);
void  iomgr_request_flush(void);
void  blk_mq_lock(int deviceid);
void  blk_mq_unlock(int deviceid);
IOrequest *IOmgr_obtainjob(int deviceid, u64 near_lba);
int   dex32_IOcomplete(DWORD handle);
void  dex32_closeIO(DWORD handle);
DWORD dex32_requestIO(int deviceid, int type, u64 block, DWORD numblocks, void *buf);

#endif
