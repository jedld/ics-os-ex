/*
  Name: gpt.c
  Description: UEFI GUID Partition Table (GPT) detection and parser.

  See gpt.h for the contract. The parser is fail-closed: a GPT is accepted
  only when a header and its partition-entry array both validate. A corrupt
  primary triggers a backup-header fallback (with a diagnostic flag); if both
  fail, no GPT is reported and the caller falls back to MBR / unpartitioned.
*/
#include "gpt.h"
#include "crc32.h"

/* ---- self-contained byte primitives -----------------------------------
   The kernel is freestanding and does not guarantee string.h, so the parser
   carries its own memcpy/memset/memcmp to stay portable across the kernel
   build and the host unit-test build. */

static void gpt_memset(void *dst, int c, unsigned int n)
{
    unsigned char *d = (unsigned char *)dst;
    while (n--)
        *d++ = (unsigned char)c;
}

static void *gpt_memcpy(void *dst, const void *src, unsigned int n)
{
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    while (n--)
        *d++ = *s++;
    return dst;
}

static int gpt_memcmp(const void *a, const void *b, unsigned int n)
{
    const unsigned char *x = (const unsigned char *)a;
    const unsigned char *y = (const unsigned char *)b;
    while (n--) {
        if (*x != *y)
            return (int)*x - (int)*y;
        x++;
        y++;
    }
    return 0;
}

/* ---- little-endian field readers (GPT is little-endian on disk) ------- */

static unsigned int gpt_le32(const unsigned char *p)
{
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8) |
           ((unsigned int)p[2] << 16) | ((unsigned int)p[3] << 24);
}

static unsigned long long gpt_le64(const unsigned char *p)
{
    return (unsigned long long)gpt_le32(p) |
           ((unsigned long long)gpt_le32(p + 4) << 32);
}

/* ---- well-known partition type GUIDs (on-disk byte order) ------------- */

typedef struct {
    const unsigned char guid[16];
    const char *name;
} gpt_type;

static const gpt_type gpt_types[] = {
    { { 0x28,0x73,0x2A,0xC1, 0x1F,0xF8, 0xD2,0x11, 0xBA,0x4B,0x00,0xA0,0xC9,0x3E,0xC9,0x3B },
      "EFI System" },                                   /* C12A7328-F81F-11D2-BA4B-00A0C93EC93B */
    { { 0xEA,0xAD,0x0A,0xEB, 0xDD,0xB2, 0x3F,0x4F, 0x80,0xFF,0x71,0xE9,0x16,0xB3,0x9E,0x0F },
      "Basic Data" },                                   /* EB0AADEA-B2DD-4F3F-80FF-71E916B39E0F */
    { { 0x16,0xE3,0xC9,0xE3, 0x0B,0x5C, 0xB8,0x4D, 0x81,0x7D,0xF9,0x2D,0xF0,0x02,0x15,0xAE },
      "Microsoft Reserved" },                           /* E3C9E316-0B5C-4DB8-817D-F92DF00215AE */
    { { 0x6D,0xFD,0x57,0x06, 0xAB,0xA4, 0xC4,0x43, 0x84,0xE5,0x09,0x33,0xC8,0x4B,0x4F,0x4F },
      "Linux swap" },                                   /* 0657FD6D-A4AB-43C4-84E5-0933C84B4F4F */
    { { 0xAF,0x3D,0xC6,0x0F, 0x83,0x84, 0x72,0x47, 0x8E,0x79,0x3D,0x69,0xD8,0x47,0x7D,0xE4 },
      "Linux data" },                                   /* 0FC63DAF-8483-4772-8E79-3D69D8477DE4 */
    { { 0x40,0x95,0x47,0x44, 0x97,0xF2, 0xB2,0x41, 0x9A,0xF7,0xD1,0x31,0xD5,0xF0,0x45,0x8A },
      "Linux root (x86)" }                              /* 44479540-F297-41B2-9AF7-D131D5F0458A */
};
#define GPT_TYPE_COUNT (sizeof(gpt_types) / sizeof(gpt_types[0]))

static const char *gpt_type_name(const unsigned char guid[16])
{
    int i;
    for (i = 0; i < (int)GPT_TYPE_COUNT; i++)
        if (gpt_memcmp(guid, gpt_types[i].guid, 16) == 0)
            return gpt_types[i].name;
    return 0;
}

/* ---- header decoding/validation --------------------------------------- */

typedef struct {
    unsigned long long my_lba;
    unsigned long long alt_lba;
    unsigned long long first_usable;
    unsigned long long last_usable;
    unsigned char disk_guid[16];
    unsigned long long entry_lba;
    unsigned int num_entries;
    unsigned int entry_size;
    unsigned int array_crc;
} gpt_header_geom;

/* Returns 1 when the 512-byte sector at `hdr` is a structurally valid GPT
   header located at `my_lba_expected`. Fills `g` with the geometry. */
static int gpt_check_header(const unsigned char hdr[512],
                            unsigned long long total_blocks,
                            unsigned long long my_lba_expected,
                            gpt_header_geom *g)
{
    unsigned int stored_crc;
    unsigned int computed_crc;
    unsigned char crc_hdr[512];

    if (gpt_memcmp(hdr, "EFI PART", 8) != 0)
        return 0;
    if (gpt_le32(hdr + 8) != GPT_REVISION)
        return 0;
    if (gpt_le32(hdr + 12) != GPT_HEADER_SIZE)
        return 0;

    stored_crc = gpt_le32(hdr + 16);
    gpt_memcpy(crc_hdr, hdr, 512);
    crc_hdr[16] = crc_hdr[17] = crc_hdr[18] = crc_hdr[19] = 0;
    computed_crc = crc32_ieee(crc_hdr, 0, GPT_HEADER_SIZE);
    if (computed_crc != stored_crc)
        return 0;

    g->my_lba        = gpt_le64(hdr + 24);
    g->alt_lba       = gpt_le64(hdr + 32);
    g->first_usable  = gpt_le64(hdr + 40);
    g->last_usable   = gpt_le64(hdr + 48);
    gpt_memcpy(g->disk_guid, hdr + 56, 16);
    g->entry_lba     = gpt_le64(hdr + 72);
    g->num_entries   = gpt_le32(hdr + 80);
    g->entry_size    = gpt_le32(hdr + 84);
    g->array_crc     = gpt_le32(hdr + 88);

    if (g->my_lba != my_lba_expected)
        return 0;
    if (g->num_entries < 1 || g->num_entries > GPT_MAX_ENTRIES)
        return 0;
    if (g->entry_size != GPT_ENTRY_SIZE)
        return 0;
    if (g->first_usable >= g->last_usable)
        return 0;
    if (g->first_usable < 1 || g->last_usable >= total_blocks)
        return 0;

    /* the entry array must fit on the disk */
    {
        unsigned long long array_bytes =
            (unsigned long long)g->num_entries * (unsigned long long)g->entry_size;
        unsigned long long array_end = g->entry_lba * 512 + array_bytes;
        if (array_end > total_blocks * 512)
            return 0;
    }
    return 1;
}

/* ---- entry-array read/validation/parsing ------------------------------ */

static unsigned char gpt_entry_buf[GPT_MAX_ENTRIES * GPT_ENTRY_SIZE];

/* Decode a UTF-16LE name (122 bytes) to printable ASCII, truncating at the
   first NUL or first non-ASCII code unit. */
static void gpt_decode_name(const unsigned char src[122], char dst[GPT_NAME_MAX])
{
    int i;
    int n = 0;
    for (i = 0; i + 1 < 122 && n < GPT_NAME_MAX - 1; i += 2) {
        unsigned short u = (unsigned short)(src[i] | (src[i + 1] << 8));
        if (u == 0)
            break;
        if (u < 0x20 || u > 0x7E)
            break;
        dst[n++] = (char)u;
    }
    dst[n] = 0;
}

/* Read the entry array described by `g`, validate its CRC, and copy the
   in-bounds entries into `disk`. Returns 1 when the array CRC is valid and
   at least the read succeeded; the number of valid entries is stored in
   `*out_count`. */
static int gpt_parse_array(gpt_read_fn read, void *arg,
                           const gpt_header_geom *g,
                           gpt_disk *disk, int *out_count)
{
    unsigned long long array_bytes;
    unsigned int sectors;
    unsigned int stored_crc;
    unsigned int i;
    int count = 0;

    array_bytes = (unsigned long long)g->num_entries * (unsigned long long)g->entry_size;
    sectors = (unsigned int)((array_bytes + 511) / 512);

    if (!read(g->entry_lba, gpt_entry_buf, sectors, arg))
        return 0;

    stored_crc = crc32_ieee(gpt_entry_buf, 0, (unsigned int)array_bytes);
    if (stored_crc != g->array_crc)
        return 0;

    for (i = 0; i < g->num_entries; i++) {
        const unsigned char *e = gpt_entry_buf + (unsigned long)i * GPT_ENTRY_SIZE;
        gpt_entry *out;
        unsigned long long first, last;
        int j;

        /* an all-zero type GUID marks an empty slot */
        {
            int empty = 1;
            for (j = 0; j < 16; j++)
                if (e[j] != 0) { empty = 0; break; }
            if (empty)
                continue;
        }

        first = gpt_le64(e + 32);
        last  = gpt_le64(e + 40);

        /* in-bounds: within the usable range and sane */
        if (first < g->first_usable || last > g->last_usable || last < first)
            continue;

        out = &disk->entries[count];
        out->index = (int)i;
        gpt_memcpy(out->type_guid, e, 16);
        gpt_memcpy(out->unique_guid, e + 16, 16);
        out->attributes = gpt_le64(e + 48);
        out->first_lba = first;
        out->last_lba = last;
        out->type_name = gpt_type_name(out->type_guid);
        gpt_decode_name(e + 56, out->name);
        count++;
        if (count >= GPT_MAX_ENTRIES)
            break;
    }

    disk->entry_count = count;
    if (out_count)
        *out_count = count;
    return 1;
}

/* ---- public API -------------------------------------------------------- */

int gpt_detect(gpt_read_fn read, void *arg)
{
    static unsigned char mbr[512];
    int i;

    if (!read(0, mbr, 1, arg))
        return -1;
    if (mbr[510] != 0x55 || mbr[511] != 0xAA)
        return -1;
    for (i = 0; i < 4; i++)
        if (mbr[446 + i * 16 + 4] == GPT_PROTECTIVE_MBR)
            return 1;
    return 0;
}

int gpt_parse(gpt_read_fn read, void *arg, unsigned long long total_blocks,
              gpt_disk *disk)
{
    static unsigned char hdr[512];
    gpt_header_geom g;
    int prim_ok = 0, prim_array_ok = 0;
    int b_ok = 0, b_array_ok = 0;
    unsigned long long backup_lba;

    gpt_memset(disk, 0, sizeof(*disk));
    if (!total_blocks)
        return 0;

    /* ---- primary header at LBA 1 ---- */
    if (read(1, hdr, 1, arg))
        prim_ok = gpt_check_header(hdr, total_blocks, 1, &g);
    disk->primary_header_ok = prim_ok;

    if (prim_ok) {
        gpt_memcpy(disk->disk_guid, g.disk_guid, 16);
        disk->first_usable_lba = g.first_usable;
        disk->last_usable_lba = g.last_usable;
        disk->my_lba = g.my_lba;
        prim_array_ok = gpt_parse_array(read, arg, &g, disk, 0);
        disk->primary_array_ok = prim_array_ok;
        if (prim_array_ok) {
            disk->valid = 1;
            disk->used_backup = 0;
            return 1;
        }
    }

    /* ---- backup header fallback ---- */
    backup_lba = prim_ok ? g.alt_lba : (total_blocks - 1);
    if (backup_lba >= 1 && backup_lba < total_blocks && backup_lba != 1) {
        if (read(backup_lba, hdr, 1, arg))
            b_ok = gpt_check_header(hdr, total_blocks, backup_lba, &g);
        disk->backup_header_ok = b_ok;
        if (b_ok) {
            gpt_memcpy(disk->disk_guid, g.disk_guid, 16);
            disk->first_usable_lba = g.first_usable;
            disk->last_usable_lba = g.last_usable;
            disk->my_lba = g.my_lba;
            b_array_ok = gpt_parse_array(read, arg, &g, disk, 0);
            disk->backup_array_ok = b_array_ok;
            if (b_array_ok) {
                disk->valid = 1;
                disk->used_backup = 1;
                return 1;
            }
        }
    }

    disk->valid = 0;
    return 0;
}
