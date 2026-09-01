/*
 * ext4.h - ext4 filesystem driver (extents + metadata checksums)
 *
 * On-disk structures are little-endian and read directly on x86 (no byte
 * swapping). Fixed-width u8/u16/u32/u64 are used for all on-disk fields.
 */

#ifndef EXT4_H
#define EXT4_H

#include "../dextypes.h"
#include "../vfs/vfs_core.h"

/* ------------------------------------------------------------------ */
/* magic numbers and feature bits                                      */
/* ------------------------------------------------------------------ */
#define EXT4_SUPER_MAGIC      0xEF53
#define EXT4_EXTENT_MAGIC     0xF30A

/* s_feature_compat */
#define EXT4_FEATURE_COMPAT_HAS_JOURNAL   0x0004
#define EXT4_FEATURE_COMPAT_EXT_ATTR      0x0008
#define EXT4_FEATURE_COMPAT_RESIZE_INODE  0x0010
#define EXT4_FEATURE_COMPAT_DIR_INDEX     0x0020

/* s_feature_ro_compat */
#define EXT4_FEATURE_RO_COMPAT_SPARSE_SUPER 0x0001
#define EXT4_FEATURE_RO_COMPAT_LARGE_FILE   0x0002
#define EXT4_FEATURE_RO_COMPAT_HUGE_FILE    0x0008
#define EXT4_FEATURE_RO_COMPAT_DIR_NLINK    0x0020
#define EXT4_FEATURE_RO_COMPAT_EXTRA_ISIZE  0x0040
#define EXT4_FEATURE_RO_COMPAT_METADATA_CSUM 0x0400

/* s_feature_incompat */
#define EXT4_FEATURE_INCOMPAT_FILETYPE      0x0002
#define EXT4_FEATURE_INCOMPAT_EXTENTS       0x0040
#define EXT4_FEATURE_INCOMPAT_64BIT         0x0080
#define EXT4_FEATURE_INCOMPAT_FLEX_BG       0x0200
#define EXT4_FEATURE_INCOMPAT_ENCRYPTION    0x1000
#define EXT4_FEATURE_INCOMPAT_BIGALLOC      0x20000
#define EXT4_FEATURE_INCOMPAT_CSUM_SEED     0x02000

/* inode i_mode */
#define EXT4_S_IFMT     0xF000
#define EXT4_S_IFSOCK   0x1000
#define EXT4_S_IFLNK    0xA000
#define EXT4_S_IFREG    0x8000
#define EXT4_S_IFBLK    0x6000
#define EXT4_S_IFDIR    0x4000
#define EXT4_S_IFCHR    0x2000
#define EXT4_S_IFIFO    0x1000

/* inode i_flags */
#define EXT4_INDEX_FL       0x1000  /* hashed (htree) directory */
#define EXT4_EXTENTS_FL     0x0008  /* extents (also set by INCOMPAT) */

/* directory entry file types */
#define EXT4_FT_UNKNOWN     0
#define EXT4_FT_REG_FILE    1
#define EXT4_FT_DIR         2
#define EXT4_FT_CHRDEV      3
#define EXT4_FT_BLKDEV      4
#define EXT4_FT_FIFO        5
#define EXT4_FT_SOCK        6
#define EXT4_FT_SYMLINK     7

/* ------------------------------------------------------------------ */
/* on-disk structures (packed, little-endian)                         */
/* ------------------------------------------------------------------ */

/* 8 + name_len; rec_len is a multiple of 4 and >= 8 + name_len */
typedef struct __attribute__((packed)) {
    u32 inode;
    u16 rec_len;
    u8  name_len;
    u8  file_type;
    char name[1];
} ext4_dirent;

/* extent tree header, first 12 bytes of a node / i_block */
typedef struct __attribute__((packed)) {
    u16 eh_magic;
    u16 eh_entries;
    u16 eh_max;
    u16 eh_depth;
    u16 eh_unused;
    u32 eh_generation;
} ext4_extent_header;

/* leaf extent entry, 12 bytes */
typedef struct __attribute__((packed)) {
    u32 ee_block;
    u16 ee_len;
    u16 ee_start_hi;
    u32 ee_start_lo;
} ext4_extent;

/* interior extent index entry, 12 bytes */
typedef struct __attribute__((packed)) {
    u32 ei_block;
    u32 ei_leaf_lo;
    u16 ei_leaf_hi;
    u16 ei_unused;
} ext4_extent_idx;

/* 8-byte tail appended after extents in external extent nodes */
typedef struct __attribute__((packed)) {
    u16 et_padding;
    u32 et_checksum;
} ext4_extent_tail;

/* ------------------------------------------------------------------ */
/* driver internal state                                              */
/* ------------------------------------------------------------------ */

/* a flattened run of contiguous logical->physical blocks */
#define EXT4_MAX_RUNS   4096
#define EXT4_MAX_LEAVES 512

typedef struct {
    u32 log_block;   /* first logical block */
    u32 len;         /* number of blocks */
    u32 phys_lo;     /* first physical block (low 32) */
    u32 phys_hi;     /* first physical block (high 32) */
} ext4_run;

/* per-vfs-node cache (stored in vfs_node::misc) */
typedef struct {
    u32 ino;
    u32 size_lo;
    u32 size_hi;
    u16 mode;
    u16 links;
    u32 nblocks;      /* i_blocks, in 512-byte units */
    u32 flags;
    u32 generation;
    u32 atime;
    u32 ctime;
    u32 mtime;
    int  is_dir;
    int  inode_loaded;
    int  runs_loaded;
    u32  nruns;
    ext4_run runs[EXT4_MAX_RUNS];
    u32  leaf_blocks[EXT4_MAX_LEAVES]; /* allocated extent-node blocks */
    u32  nleaf;
    void *dirent;     /* pointer into parent dir image */
    int  dirent_valid;
    u8   inode_raw[256]; /* cached raw inode (max ext4 inode size) */
} ext4_ino;

/* parsed group descriptor */
typedef struct {
    u32 bg_block_bitmap;
    u32 bg_inode_bitmap;
    u32 bg_inode_table;
    u32 bg_block_bitmap_hi;
    u32 bg_inode_bitmap_hi;
    u32 bg_inode_table_hi;
    u16 bg_free_blocks;
    u16 bg_free_inodes;
    u16 bg_flags;
    u16 bg_checksum;
} ext4_gd;

/* parsed superblock (raw copy kept in ext4_dev::sbuf for checksums) */
typedef struct {
    u32 blocksize;
    u32 inode_size;
    u32 inodes_per_group;
    u32 blocks_per_group;
    u32 first_data_block;
    u32 desc_size;
    u64 blocks_count;
    u64 free_blocks;
    u32 free_inodes;
    u32 feature_compat;
    u32 feature_incompat;
    u32 feature_ro_compat;
    u8  uuid[16];
    u32 checksum;
    u32 csum_seed;
    int has_csum;
    int has_extents;
    int has_64bit;
    int has_flex_bg;
    int has_dir_index;
    int has_filetype;
    u32 num_groups;
    u32 gdt_block;   /* ext4 block number of the group descriptor table */
    u32 gdt_size;    /* total bytes of the GDT */
} ext4_sb;

/* per-device context (keyed by block device id) */
typedef struct {
    int devid;
    int mounted;
    u8  sbuf[1024];
    ext4_sb sb;
    u8   *gdt_raw;
    ext4_gd *gdt;
    u32   alloc_block_hint;
    u32   alloc_ino_hint;
} ext4_dev;

/* ------------------------------------------------------------------ */
/* driver API                                                         */
/* ------------------------------------------------------------------ */
int ext4_register(const char *name);
int ext4_identify(int device);

#endif /* EXT4_H */
