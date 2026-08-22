/*
 * In-kernel RAM disk block device (replaces the i386 PE ramdisk.dll on x86_64).
 * Provides a FAT16-formatted memory image mounted at /ramdisk for selfhost.
 */
#include "../dextypes.h"
#include "../devmgr/dex32_devmgr.h"
#include "../vfs/vfs_core.h"
#include "ramdisk.h"

extern void *malloc(unsigned int);
extern void printf(const char *fmt, ...);
extern void *memset(void *s, int c, unsigned int n);
extern void *memcpy(void *d, const void *s, unsigned int n);
extern char *strcpy(char *d, const char *s);

#define RAMDISK_SECTORS   16384  /* 8 MiB — enough for tccboot sources + output */
#define RAMDISK_SECSIZE   512
#define RAMDISK_BYTES     (RAMDISK_SECTORS * RAMDISK_SECSIZE)

static unsigned char *ramdisk_data;
static int ramdisk_devid = -1;

static int ramdisk_read_block(int block, char *buf, int numblocks)
{
   DWORD off;
   if (!ramdisk_data || block < 0 || numblocks <= 0)
      return -1;
   if ((DWORD)block + (DWORD)numblocks > RAMDISK_SECTORS)
      return -1;
   off = (DWORD)block * RAMDISK_SECSIZE;
   memcpy(buf, ramdisk_data + off, (DWORD)numblocks * RAMDISK_SECSIZE);
   return 1;
}

static int ramdisk_write_block(int block, char *buf, int numblocks)
{
   DWORD off;
   if (!ramdisk_data || block < 0 || numblocks <= 0)
      return -1;
   if ((DWORD)block + (DWORD)numblocks > RAMDISK_SECTORS)
      return -1;
   off = (DWORD)block * RAMDISK_SECSIZE;
   memcpy(ramdisk_data + off, buf, (DWORD)numblocks * RAMDISK_SECSIZE);
   return 1;
}

static int ramdisk_get_block_size(void)
{
   return RAMDISK_SECSIZE;
}

static int ramdisk_total_blocks(void)
{
   return RAMDISK_SECTORS;
}

/* Minimal FAT16 BPB + empty FATs + empty root directory. */
static void ramdisk_format_fat16(void)
{
   unsigned char *b = ramdisk_data;
   int i;
   int reserved = 1;
   int fats = 2;
   int root_ents = 512;
   int fat_secs = 32;

   memset(b, 0, RAMDISK_BYTES);

   b[0] = 0xEB; b[1] = 0x3C; b[2] = 0x90;
   memcpy(b + 3, "ICSOSRAM", 8);
   b[11] = RAMDISK_SECSIZE & 0xFF;
   b[12] = (RAMDISK_SECSIZE >> 8) & 0xFF;
   b[13] = 1;
   b[14] = reserved & 0xFF;
   b[15] = (reserved >> 8) & 0xFF;
   b[16] = fats;
   b[17] = root_ents & 0xFF;
   b[18] = (root_ents >> 8) & 0xFF;
   b[19] = RAMDISK_SECTORS & 0xFF;
   b[20] = (RAMDISK_SECTORS >> 8) & 0xFF;
   b[21] = 0xF8;
   b[22] = fat_secs & 0xFF;
   b[23] = (fat_secs >> 8) & 0xFF;
   b[24] = 32; b[25] = 0;
   b[26] = 2;  b[27] = 0;
   b[36] = 0x80;
   b[38] = 0x29;
   b[39] = 0x49; b[40] = 0x43; b[41] = 0x53; b[42] = 0x4F;
   memcpy(b + 43, "ICSOS_RAM  ", 11);
   memcpy(b + 54, "FAT16   ", 8);
   b[510] = 0x55;
   b[511] = 0xAA;

   for (i = 0; i < fats; i++) {
      unsigned char *fat = b + (reserved + i * fat_secs) * RAMDISK_SECSIZE;
      fat[0] = 0xF8;
      fat[1] = 0xFF;
      fat[2] = 0xFF;
      fat[3] = 0xFF;
   }
}

static int ramdisk_getcache(char *buf, DWORD block, DWORD numblocks)
{
   return ramdisk_read_block((int)block, buf, (int)numblocks) == 1;
}

static int ramdisk_putcache(char *buf, DWORD block, DWORD numblocks)
{
   return ramdisk_write_block((int)block, buf, (int)numblocks) == 1;
}

void ramdisk_init(void)
{
   devmgr_block_desc myblock;

   ramdisk_data = (unsigned char *)malloc(RAMDISK_BYTES);
   if (!ramdisk_data) {
      printf("ramdisk: alloc failed\n");
      return;
   }
   ramdisk_format_fat16();

   memset(&myblock, 0, sizeof(myblock));
   strcpy(myblock.hdr.name, "ramdisk");
   strcpy(myblock.hdr.description, "In-kernel FAT16 RAM disk");
   myblock.hdr.type = DEVMGR_BLOCK;
   myblock.hdr.size = sizeof(myblock);
   myblock.read_block = ramdisk_read_block;
   myblock.write_block = ramdisk_write_block;
   myblock.get_block_size = ramdisk_get_block_size;
   myblock.total_blocks = ramdisk_total_blocks;
   /* Complete I/O inline so selfhost does not depend on disk_mgr. */
   myblock.getcache = ramdisk_getcache;
   myblock.putcache = ramdisk_putcache;

   ramdisk_devid = devmgr_register((devmgr_generic *)&myblock);
   if (ramdisk_devid == -1)
      printf("ramdisk: register failed\n");
   else
      printf("ramdisk: %d KiB ready\n", RAMDISK_BYTES / 1024);
}

void ramdisk_mount(void)
{
   if (ramdisk_devid == -1)
      return;
   if (vfs_mount_device("fat", "ramdisk", "ramdisk") == -1)
      printf("ramdisk: mount failed\n");
   else
      printf("ramdisk: mounted at /ramdisk\n");
}
