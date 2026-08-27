#ifndef _BLKCACHE_H
#define _BLKCACHE_H

#include "../types.h"

void blkcache_init(void);
int  blkcache_get(int deviceid, u64 sector, DWORD numblocks, void *buf);
int  blkcache_put(int deviceid, u64 sector, DWORD numblocks, void *buf);
void blkcache_invalidate_device(int deviceid);
void blkcache_stats(DWORD *hits, DWORD *misses, DWORD *fills, DWORD *slots);
void blkcache_reset_stats(void);

#endif
