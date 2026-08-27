#ifndef _BLKCACHE_H
#define _BLKCACHE_H

#include "../types.h"

/* 4KiB page cache (P2). Index is (device, byte_offset >> 12). Dirty pages
   are written back by blkcache_flush() from disk_mgr / fclose. */

void blkcache_init(void);
int  blkcache_get(int deviceid, u64 sector, DWORD numblocks, void *buf);
int  blkcache_put(int deviceid, u64 sector, DWORD numblocks, void *buf);
int  blkcache_read(int deviceid, u64 sector, DWORD numblocks, void *buf);
int  blkcache_write(int deviceid, u64 sector, DWORD numblocks, void *buf);
int  blkcache_flush(void);
int  blkcache_has_dirty(void);
void blkcache_invalidate_device(int deviceid);
void blkcache_stats(DWORD *hits, DWORD *misses, DWORD *fills, DWORD *slots);
void blkcache_reset_stats(void);
void blkcache_mq_stats(DWORD *merged);

#endif
