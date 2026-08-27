/*
 * Generic LRU block cache for ICS-OS block devices.
 * Device-aware (deviceid + LBA). Used so iosched can satisfy repeated
 * reads (FAT tables, BPB, hot executables) without hitting the media.
 *
 * Algorithm: open-addressed hash of (dev,lba) with linear probing;
 * collisions overwrite the first probe slot (CLOCK-ish reuse).
 */
#include "../dextypes.h"
#include "../process/sync.h"
#include "blkcache.h"

extern void *malloc(unsigned int);
extern void free(void*);
extern void *memcpy(void*, const void*, unsigned int);
extern void *memset(void*, int, unsigned int);

#define BLKCACHE_SIZE   1024   /* 512 KiB */
#define BLKCACHE_BSIZE  512

typedef struct {
   int    deviceid;
   u64    lba;
   DWORD  age;
   BYTE   valid;
   BYTE   data[BLKCACHE_BSIZE];
} blkcache_entry;

static blkcache_entry *blkcache_tab;
static DWORD blkcache_clock;
static DWORD blkcache_hits;
static DWORD blkcache_misses;
static DWORD blkcache_fills;
static sync_sharedvar blkcache_busy;

void blkcache_init(void)
{
   int i;
   memset(&blkcache_busy, 0, sizeof(blkcache_busy));
   blkcache_tab = (blkcache_entry*)malloc(sizeof(blkcache_entry) * BLKCACHE_SIZE);
   if (!blkcache_tab) return;
   for (i = 0; i < BLKCACHE_SIZE; i++) {
      blkcache_tab[i].valid = 0;
      blkcache_tab[i].deviceid = -1;
   }
   blkcache_clock = 1;
   blkcache_hits = blkcache_misses = blkcache_fills = 0;
}

static int blkcache_slot(int deviceid, u64 lba)
{
   DWORD h = (DWORD)lba * 2654435761u + (DWORD)(lba >> 32) * 2246822519u
             + (DWORD)deviceid * 40503u;
   return (int)(h % BLKCACHE_SIZE);
}

int blkcache_get(int deviceid, u64 sector, DWORD numblocks, void *buf)
{
   DWORD i;
   char *dst = (char*)buf;
   if (!blkcache_tab || numblocks == 0) return 0;

   sync_entercrit(&blkcache_busy);
   for (i = 0; i < numblocks; i++) {
      int s = blkcache_slot(deviceid, sector + i);
      int found = 0;
      int probe;
      /* Linear probe a few slots for collisions. */
      for (probe = 0; probe < 8; probe++) {
         int idx = (s + probe) % BLKCACHE_SIZE;
         if (blkcache_tab[idx].valid &&
             blkcache_tab[idx].deviceid == deviceid &&
             blkcache_tab[idx].lba == sector + i) {
            memcpy(dst + i * BLKCACHE_BSIZE, blkcache_tab[idx].data, BLKCACHE_BSIZE);
            blkcache_tab[idx].age = ++blkcache_clock;
            found = 1;
            break;
         }
      }
      if (!found) {
         blkcache_misses++;
         sync_leavecrit(&blkcache_busy);
         return 0;
      }
   }
   blkcache_hits += numblocks;
   sync_leavecrit(&blkcache_busy);
   return 1;
}

/* Store sectors into the cache (after a media read or write). */
int blkcache_put(int deviceid, u64 sector, DWORD numblocks, void *buf)
{
   DWORD i;
   char *src = (char*)buf;
   if (!blkcache_tab || numblocks == 0) return 0;

   sync_entercrit(&blkcache_busy);
   for (i = 0; i < numblocks; i++) {
      int s = blkcache_slot(deviceid, sector + i);
      int idx = s;
      int probe;
      /* Prefer empty/matching slot; else overwrite first probe. */
      for (probe = 0; probe < 8; probe++) {
         int j = (s + probe) % BLKCACHE_SIZE;
         if (!blkcache_tab[j].valid ||
             (blkcache_tab[j].deviceid == deviceid &&
              blkcache_tab[j].lba == sector + i)) {
            idx = j;
            break;
         }
      }
      memcpy(blkcache_tab[idx].data, src + i * BLKCACHE_BSIZE, BLKCACHE_BSIZE);
      blkcache_tab[idx].deviceid = deviceid;
      blkcache_tab[idx].lba = sector + i;
      blkcache_tab[idx].valid = 1;
      blkcache_tab[idx].age = ++blkcache_clock;
      blkcache_fills++;
   }
   sync_leavecrit(&blkcache_busy);
   return 1;
}

void blkcache_invalidate_device(int deviceid)
{
   int i;
   if (!blkcache_tab) return;
   sync_entercrit(&blkcache_busy);
   for (i = 0; i < BLKCACHE_SIZE; i++)
      if (blkcache_tab[i].deviceid == deviceid)
         blkcache_tab[i].valid = 0;
   sync_leavecrit(&blkcache_busy);
}

void blkcache_stats(DWORD *hits, DWORD *misses, DWORD *fills, DWORD *slots)
{
   if (hits) *hits = blkcache_hits;
   if (misses) *misses = blkcache_misses;
   if (fills) *fills = blkcache_fills;
   if (slots) *slots = BLKCACHE_SIZE;
}

void blkcache_reset_stats(void)
{
   blkcache_hits = blkcache_misses = blkcache_fills = 0;
}
