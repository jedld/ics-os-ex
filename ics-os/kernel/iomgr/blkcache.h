#ifndef _BLKCACHE_H
#define _BLKCACHE_H

#include "../dextypes.h"

void blkcache_init(void);
int  blkcache_get(int deviceid, DWORD sector, DWORD numblocks, void *buf);
int  blkcache_put(int deviceid, DWORD sector, DWORD numblocks, void *buf);
void blkcache_invalidate_device(int deviceid);
void blkcache_stats(DWORD *hits, DWORD *misses, DWORD *fills, DWORD *slots);
void blkcache_reset_stats(void);

#endif
