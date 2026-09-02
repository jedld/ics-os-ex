/*
  Name: gpt.h
  Description: Detection and parsing of the UEFI GUID Partition Table (GPT).

  Self-contained (uses only fixed-width scalar types, no kernel includes) so
  the exact same source compiles in-kernel (via kernel32.c) and in host unit
  tests. LBA values use `unsigned long long`, which is the kernel's `u64` on
  every supported target.

  The module is read-only and fail-closed: it never writes to the partition
  table, and it reports a GPT as valid only when a header (primary, or backup
  fallback) and its partition-entry array both pass signature/CRC/geometry
  validation. Callers register no partitions from unvalidated metadata.
*/
#ifndef ICSOS_PARTITION_GPT_H
#define ICSOS_PARTITION_GPT_H

#define GPT_SECTOR_SIZE    512
#define GPT_HEADER_SIZE    92
#define GPT_REVISION       0x00010000u
#define GPT_MAX_ENTRIES    128
#define GPT_ENTRY_SIZE     128
#define GPT_NAME_MAX       37      /* 36 chars + NUL */
#define GPT_PROTECTIVE_MBR 0xEE    /* MBR type byte of the protective entry */

/* Read `sectors` 512-byte sectors starting at `lba` into `buf`.
   Returns 1 on success, 0 on failure. `arg` is driver-specific context. */
typedef int (*gpt_read_fn)(unsigned long long lba, void *buf,
                           unsigned int sectors, void *arg);

typedef struct {
    int index;                   /* entry number, 0-based */
    unsigned char type_guid[16];
    unsigned char unique_guid[16];
    unsigned long long attributes;
    unsigned long long first_lba;
    unsigned long long last_lba; /* inclusive */
    char name[GPT_NAME_MAX];
    const char *type_name;       /* mapped human-readable name, may be 0 */
} gpt_entry;

typedef struct {
    int valid;                   /* 1 when a validated GPT was parsed */
    int used_backup;             /* 1 when the backup header was accepted */
    unsigned char disk_guid[16];
    int entry_count;             /* number of non-empty, in-bounds entries */
    unsigned long long first_usable_lba;
    unsigned long long last_usable_lba;
    unsigned long long my_lba;   /* LBA of the header that was accepted */
    gpt_entry entries[GPT_MAX_ENTRIES];
    /* per-source validation results, exposed for diagnostics */
    int primary_header_ok;
    int primary_array_ok;
    int backup_header_ok;
    int backup_array_ok;
} gpt_disk;

/* Inspect LBA 0:
     1  -> GPT disk (MBR signature present with an 0xEE protective entry)
     0  -> plain MBR (MBR signature present, no 0xEE entry)
    -1  -> unpartitioned (no MBR signature) or read error */
int gpt_detect(gpt_read_fn read, void *arg);

/* Parse and validate the GPT. The primary header is read at LBA 1; if it
   (or its entry array) fails validation, the backup header is tried. On
   success `*disk` is filled and 1 is returned. On failure 0 is returned and
   `*disk->valid` is 0 (fail-closed); the per-source *_ok flags record which
   validations failed. `total_blocks` is the whole-disk sector count. */
int gpt_parse(gpt_read_fn read, void *arg, unsigned long long total_blocks,
              gpt_disk *disk);

#endif /* ICSOS_PARTITION_GPT_H */
