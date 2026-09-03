/*
  Name: gpt_unit.c
  Description: Host unit tests for the GPT detector/parser
  (kernel/partition/gpt.c). Synthetic in-memory disk images (no QEMU) cover
  detection, primary/backup header selection, CRC validation, per-entry LBA
  bounds, type-GUID mapping, and UTF-16LE name decoding.

  The disk model (512-byte sectors):
    LBA 0            protective / plain MBR
    LBA 1            primary GPT header
    LBA 2..33        primary entry array (128 x 128B = 32 sectors)
    LBA 991..1022    backup entry array
    LBA 1023         backup GPT header
    usable range     34 .. 990
  total_blocks = 1024.
*/
#include <stdio.h>
#include <string.h>

#include "kernel/partition/crc32.h"
#include "kernel/partition/crc32.c"
#include "kernel/partition/gpt.h"
#include "kernel/partition/gpt.c"

#define TOTAL 1024ULL
#define PRIMARY_HDR   1ULL
#define PRIMARY_ARR   2ULL
#define BACKUP_ARR    991ULL
#define BACKUP_HDR    1023ULL
#define FIRST_USABLE  34ULL
#define LAST_USABLE   990ULL
#define NENT          128

/* on-disk (little-endian mixed) byte order, matching gpt.c's type table */
static const unsigned char EFI_GUID[16]   =
    { 0x28,0x73,0x2A,0xC1, 0x1F,0xF8, 0xD2,0x11, 0xBA,0x4B,0x00,0xA0,0xC9,0x3E,0xC9,0x3B };
static const unsigned char BASIC_GUID[16] =
    { 0xEA,0xAD,0x0A,0xEB, 0xDD,0xB2, 0x3F,0x4F, 0x80,0xFF,0x71,0xE9,0x16,0xB3,0x9E,0x0F };
static const unsigned char UNIX_GUID[16]  =
    { 0xAF,0x3D,0xC6,0x0F, 0x83,0x84, 0x72,0x47, 0x8E,0x79,0x3D,0x69,0xD8,0x47,0x7D,0xE4 };

static unsigned char g_disk[TOTAL * 512];
static unsigned char g_entries[NENT * 128];

static void put_le32(unsigned char *p, unsigned int v) {
    p[0]=v&0xFF; p[1]=(v>>8)&0xFF; p[2]=(v>>16)&0xFF; p[3]=(v>>24)&0xFF;
}
static void put_le64(unsigned char *p, unsigned long long v) {
    put_le32(p, (unsigned int)(v & 0xFFFFFFFFu));
    put_le32(p+4, (unsigned int)((v >> 32) & 0xFFFFFFFFu));
}

static void build_header(unsigned char *h, unsigned long long my_lba,
                         unsigned long long alt_lba,
                         unsigned long long first_usable,
                         unsigned long long last_usable,
                         const unsigned char *disk_guid,
                         unsigned long long entry_lba,
                         unsigned int num_entries, unsigned int entry_size,
                         unsigned int array_crc)
{
    memset(h, 0, 512);
    memcpy(h, "EFI PART", 8);
    put_le32(h + 8, GPT_REVISION);
    put_le32(h + 12, GPT_HEADER_SIZE);
    put_le32(h + 16, 0);                 /* CRC placeholder */
    put_le64(h + 24, my_lba);
    put_le64(h + 32, alt_lba);
    put_le64(h + 40, first_usable);
    put_le64(h + 48, last_usable);
    memcpy(h + 56, disk_guid, 16);
    put_le64(h + 72, entry_lba);
    put_le32(h + 80, num_entries);
    put_le32(h + 84, entry_size);
    put_le32(h + 88, array_crc);
    put_le32(h + 16, crc32_ieee(h, 0, GPT_HEADER_SIZE));
}

static void build_entry(unsigned char *e, const unsigned char *type_guid,
                        const unsigned char *unique_guid,
                        unsigned long long first, unsigned long long last,
                        unsigned long long attrs, const char *name)
{
    int i;
    memset(e, 0, 128);
    memcpy(e + 0, type_guid, 16);
    memcpy(e + 16, unique_guid, 16);
    put_le64(e + 32, first);
    put_le64(e + 40, last);
    put_le64(e + 48, attrs);
    for (i = 0; name && name[i] && i < 36; i++) {
        e[56 + i * 2] = (unsigned char)name[i];
        e[56 + i * 2 + 1] = 0;
    }
}

static int fake_read(unsigned long long lba, void *buf, unsigned int sectors, void *arg)
{
    (void)arg;
    if (lba + sectors > TOTAL)
        return 0;
    memcpy(buf, g_disk + lba * 512, sectors * 512);
    return 1;
}

/* Recompute a GPT header's self-CRC the way the spec requires: the CRC field
   (offset 16) must be zeroed while the CRC is taken. */
static void recompute_header_crc(unsigned char *h)
{
    put_le32(h + 16, 0);
    put_le32(h + 16, crc32_ieee(h, 0, GPT_HEADER_SIZE));
}

/* Write the global g_entries to both arrays and refresh both headers' array
   CRC + self-CRC so the whole table stays internally consistent. */
static void install_arrays_both(void)
{
    unsigned int ac = crc32_ieee(g_entries, 0, sizeof(g_entries));
    memcpy(g_disk + PRIMARY_ARR * 512, g_entries, sizeof(g_entries));
    memcpy(g_disk + BACKUP_ARR * 512, g_entries, sizeof(g_entries));
    put_le32(g_disk + PRIMARY_HDR * 512 + 88, ac);
    recompute_header_crc(g_disk + PRIMARY_HDR * 512);
    put_le32(g_disk + BACKUP_HDR * 512 + 88, ac);
    recompute_header_crc(g_disk + BACKUP_HDR * 512);
}

/* Write g_entries to the primary array only and refresh the primary header. */
static void install_primary_array(void)
{
    unsigned int ac = crc32_ieee(g_entries, 0, sizeof(g_entries));
    memcpy(g_disk + PRIMARY_ARR * 512, g_entries, sizeof(g_entries));
    put_le32(g_disk + PRIMARY_HDR * 512 + 88, ac);
    recompute_header_crc(g_disk + PRIMARY_HDR * 512);
}

static const unsigned char DISK_GUID[16] =
    { 0xA1,0x2B,0x3C,0x4D, 0x5E,0x6F, 0x70,0x81, 0x92,0xA3,0xB4,0xC5,0xD6,0xE7,0xF8,0x09 };
static const unsigned char UNIQ0[16] =
    { 0x00,0x01,0x02,0x03, 0x04,0x05, 0x06,0x07, 0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F };
static const unsigned char UNIQ1[16] =
    { 0x10,0x11,0x12,0x13, 0x14,0x15, 0x16,0x17, 0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F };

static void build_base_disk(void)
{
    unsigned int array_crc;
    memset(g_disk, 0, sizeof(g_disk));

    /* protective MBR at LBA 0 */
    g_disk[0 * 512 + 510] = 0x55;
    g_disk[0 * 512 + 511] = 0xAA;
    g_disk[0 * 512 + 446 + 4] = GPT_PROTECTIVE_MBR;
    put_le32(g_disk + 0 * 512 + 446 + 8, 1);
    put_le32(g_disk + 0 * 512 + 446 + 12, (unsigned int)(TOTAL - 1));

    /* entry array: entry0 = EFI system, entry1 = basic data */
    memset(g_entries, 0, sizeof(g_entries));
    build_entry(g_entries + 0 * 128, EFI_GUID,   UNIQ0, FIRST_USABLE, 503, 0x8000000000000000ULL, "efi");
    build_entry(g_entries + 1 * 128, BASIC_GUID, UNIQ1, 504, LAST_USABLE, 0, "root");
    array_crc = crc32_ieee(g_entries, 0, sizeof(g_entries));

    memcpy(g_disk + PRIMARY_ARR * 512, g_entries, sizeof(g_entries));
    memcpy(g_disk + BACKUP_ARR * 512, g_entries, sizeof(g_entries));

    build_header(g_disk + PRIMARY_HDR * 512, PRIMARY_HDR, BACKUP_HDR,
                 FIRST_USABLE, LAST_USABLE, DISK_GUID, PRIMARY_ARR, NENT, 128, array_crc);
    build_header(g_disk + BACKUP_HDR * 512, BACKUP_HDR, PRIMARY_HDR,
                 FIRST_USABLE, LAST_USABLE, DISK_GUID, BACKUP_ARR, NENT, 128, array_crc);
}

static int check(const char *name, int condition)
{
    if (!condition) {
        printf("not ok - %s\n", name);
        return 0;
    }
    printf("ok - %s\n", name);
    return 1;
}

int main(void)
{
    int ok = 1;
    gpt_disk d;

    printf("TAP version 13\n1..20\n");

    /* --- detection ---------------------------------------------------- */
    build_base_disk();
    ok &= check("GPT disk (protective MBR) is detected as GPT",
                gpt_detect(fake_read, 0) == 1);

    /* plain MBR: valid signature, no 0xEE entry */
    build_base_disk();
    memset(g_disk, 0, 512);
    g_disk[510] = 0x55; g_disk[511] = 0xAA;
    g_disk[446 + 4] = 0x07;  /* NTFS, not protective */
    ok &= check("plain MBR is detected as MBR",
                gpt_detect(fake_read, 0) == 0);

    /* all-zero LBA 0 */
    memset(g_disk, 0, sizeof(g_disk));
    ok &= check("all-zero LBA 0 is detected as unpartitioned",
                gpt_detect(fake_read, 0) == -1);

    /* --- valid GPT parse --------------------------------------------- */
    build_base_disk();
    ok &= check("valid GPT parses successfully",
                gpt_parse(fake_read, 0, TOTAL, &d) == 1 && d.valid);
    ok &= check("valid GPT uses the primary header",
                d.used_backup == 0 && d.my_lba == PRIMARY_HDR);
    ok &= check("valid GPT has two non-empty entries",
                d.entry_count == 2);
    ok &= check("entry 0 type GUID maps to 'EFI System'",
                d.entries[0].type_name != 0 &&
                strcmp(d.entries[0].type_name, "EFI System") == 0);
    ok &= check("entry 0 LBA range is first_usable..503",
                d.entries[0].first_lba == FIRST_USABLE &&
                d.entries[0].last_lba == 503);
    ok &= check("entry 0 UTF-16LE name decodes to 'efi'",
                strcmp(d.entries[0].name, "efi") == 0);
    ok &= check("entry 1 type GUID maps to 'Basic Data'",
                d.entries[1].type_name != 0 &&
                strcmp(d.entries[1].type_name, "Basic Data") == 0);
    ok &= check("entry 1 LBA range is 504..last_usable",
                d.entries[1].first_lba == 504 &&
                d.entries[1].last_lba == LAST_USABLE);
    ok &= check("entry 1 UTF-16LE name decodes to 'root'",
                strcmp(d.entries[1].name, "root") == 0);
    ok &= check("disk GUID round-trips",
                memcmp(d.disk_guid, DISK_GUID, 16) == 0);
    ok &= check("usable range is decoded from the header",
                d.first_usable_lba == FIRST_USABLE &&
                d.last_usable_lba == LAST_USABLE);

    /* --- corruption / backup fallback -------------------------------- */
    build_base_disk();
    g_disk[PRIMARY_HDR * 512 + 20] ^= 0xFF;   /* corrupt primary header body */
    ok &= check("corrupt primary header CRC falls back to backup",
                gpt_parse(fake_read, 0, TOTAL, &d) == 1 && d.valid);
    ok &= check("fallback reports the backup header as used",
                d.used_backup == 1 && d.my_lba == BACKUP_HDR);

    build_base_disk();
    g_disk[PRIMARY_HDR * 512 + 20] ^= 0xFF;   /* primary header corrupt */
    g_disk[BACKUP_HDR * 512 + 20] ^= 0xFF;    /* backup header corrupt */
    ok &= check("corrupt primary and backup headers yield no GPT",
                gpt_parse(fake_read, 0, TOTAL, &d) == 0 && !d.valid);

    build_base_disk();
    /* corrupt one byte of the on-disk primary array *after* its CRC was
       computed, so the header's array CRC no longer matches; the header itself
       stays valid and the parser must fall back to the intact backup. */
    g_disk[PRIMARY_ARR * 512 + 0] ^= 0xFF;
    ok &= check("corrupt primary entry array falls back to backup",
                gpt_parse(fake_read, 0, TOTAL, &d) == 1 &&
                d.used_backup == 1);

    /* --- per-entry bounds -------------------------------------------- */
    build_base_disk();
    /* push entry 1 past the usable range; entry 0 stays valid */
    put_le64(g_entries + 1 * 128 + 40, LAST_USABLE + 1000);
    install_arrays_both();
    ok &= check("out-of-bounds entry is skipped",
                gpt_parse(fake_read, 0, TOTAL, &d) == 1 &&
                d.entry_count == 1);

    /* --- name decoding truncation ------------------------------------ */
   build_base_disk();
    {
        unsigned char *e = g_entries + 0 * 128 + 56;
        /* "ab" then U+00E9 (non-ASCII) then "cd": must truncate at 'b' */
        e[0] = 'a'; e[1] = 0;
        e[2] = 'b'; e[3] = 0;
        e[4] = 0xE9; e[5] = 0x00;
        e[6] = 'c'; e[7] = 0;
        e[8] = 'd'; e[9] = 0;
        install_primary_array();
    }
    ok &= check("non-ASCII name truncates at the first non-ASCII unit",
                gpt_parse(fake_read, 0, TOTAL, &d) == 1 &&
                strcmp(d.entries[0].name, "ab") == 0);

    (void)UNIX_GUID;
    return ok ? 0 : 1;
}
