/*
 * ext4.c - ext4 filesystem driver for ICS-OS
 *
 * Supports: extents (inline + multi-level), 1K/2K/4K blocks, 64-bit
 * block numbers, metadata checksums (CRC32C), hashed-directory (htree)
 * reads via linear scan, block/inode allocation, and extent-tree
 * writeback with checksums.
 *
 * The per-device context is keyed by block device id. The per-node
 * cache lives in vfs_node::misc; a directory image lives in
 * vfs_node::misc2. The VFS frees those buffers when the sizes are
 * nonzero, so this driver owns exactly the memory it puts there.
 */

#include "../dextypes.h"
#include "ext4.h"
#include "../devmgr/dex32_devmgr.h"
#include "../iomgr/iosched.h"
#include "../vfs/vfs_core.h"

extern void taskswitch(void);

/* ------------------------------------------------------------------ */
/* per-device context registry                                         */
/* ------------------------------------------------------------------ */
#define EXT4_MAX_DEVS 32

static ext4_dev *ext4_devs[EXT4_MAX_DEVS];

static ext4_dev *ext4_get_dev(int id)
{
    if (id < 0 || id >= EXT4_MAX_DEVS) return 0;
    return ext4_devs[id];
}

/* ------------------------------------------------------------------ */
/* CRC32C (Castagnoli, reflected poly 0x82F63B78, no final XOR)        */
/* ------------------------------------------------------------------ */
static u32 ext4_crc_tab[256];
static int ext4_crc_ready = 0;

static void ext4_crc_init(void)
{
    int i, j;
    u32 c;
    for (i = 0; i < 256; i++)
    {
        c = (u32)i;
        for (j = 0; j < 8; j++)
            c = (c >> 1) ^ ((c & 1) ? 0x82F63B78 : 0);
        ext4_crc_tab[i] = c;
    }
    ext4_crc_ready = 1;
}

/* kernel ext4_chksum() semantics: process data starting from crc, no
   final XOR. */
static u32 ext4_crc32c(const void *data, u32 crc, u32 len)
{
    const u8 *p = (const u8 *)data;
    u32 c = crc;
    u32 i;
    if (!ext4_crc_ready) ext4_crc_init();
    for (i = 0; i < len; i++)
        c = ext4_crc_tab[(c ^ p[i]) & 0xFF] ^ (c >> 8);
    return c;
}

/* ------------------------------------------------------------------ */
/* small field accessors (little-endian on-disk)                       */
/* ------------------------------------------------------------------ */
static u16 rd16(const u8 *p) { return (u16)(p[0] | (p[1] << 8)); }
static u32 rd32(const u8 *p) { return (u32)(p[0] | (p[1] << 8) | (p[2] << 16) | ((u32)p[3] << 24)); }
static void wr16(u8 *p, u16 v) { p[0] = (u8)(v & 0xFF); p[1] = (u8)((v >> 8) & 0xFF); }
static void wr32(u8 *p, u32 v)
{
    p[0] = (u8)(v & 0xFF);
    p[1] = (u8)((v >> 8) & 0xFF);
    p[2] = (u8)((v >> 16) & 0xFF);
    p[3] = (u8)((v >> 24) & 0xFF);
}

static void le32buf(u8 *p, u32 v) { wr32(p, v); }

/* forward declarations (used before their definition) */
static u32 ext4_alloc_block_raw(ext4_dev *d, int id);
static void ext4_free_block_raw(ext4_dev *d, int id, u32 block);
static int ext4_write_extent_leaf(ext4_dev *d, int id, u32 phys,
                                  ext4_run *runs, u32 start, u32 count,
                                  u32 gen, u32 ino);
static ext4_dirent *ext4_alloc_entry(ext4_dev *d, u8 *img, u32 *bytes,
                                     u32 ino, const char *name, u8 file_type);
static void ext4_write_parent_dir(ext4_dev *d, int id, vfs_node *parent,
                                  u8 *img, u32 bytes);
static void ext4_merge_delete(ext4_dev *d, int id, vfs_node *parent,
                              u8 *img, ext4_dirent *e);
static int ext4_free_ino_raw(ext4_dev *d, int id, u32 ino);

/* ------------------------------------------------------------------ */
/* block I/O (ext4 block number -> 512-byte LBA)                       */
/* ------------------------------------------------------------------ */
static void ext4_wait(DWORD h) { while (!dex32_IOcomplete(h)) taskswitch(); }

static int ext4_read_blocks(ext4_dev *d, int id, u64 block, u32 count, void *buf)
 {
     u32 spb = (u32)(d->sb.blocksize / 512);
     u64 lba = block * (u64)spb;
     DWORD h = dex32_requestIO(id, IO_READ, lba, (DWORD)(count * spb), buf);
    ext4_wait(h);
      dex32_closeIO(h);
      return 0;
 }

static int ext4_write_blocks(ext4_dev *d, int id, u64 block, u32 count, void *buf)
{
    u32 spb = (u32)(d->sb.blocksize / 512);
    u64 lba = block * (u64)spb;
    DWORD h = dex32_requestIO(id, IO_WRITE, lba, (DWORD)(count * spb), buf);
    ext4_wait(h);
    dex32_closeIO(h);
    return 0;
}

/* ------------------------------------------------------------------ */
/* superblock load + verify                                            */
/* ------------------------------------------------------------------ */
static int ext4_load_super(ext4_dev *d, int id)
{
    ext4_sb *s = &d->sb;
    u8 *sb = d->sbuf;
    u32 gdt_off;

  /* superblock is always at byte 1024 = sectors 2..3 (1024 bytes),
      independent of the filesystem block size. */
   {
       DWORD h = dex32_requestIO(id, IO_READ, 2, 2, sb);
       ext4_wait(h);
       dex32_closeIO(h);
   }

   if (rd16(sb + 0x38) != EXT4_SUPER_MAGIC) return -1;

    s->blocksize = 1024u << rd32(sb + 0x18);
    if (s->blocksize < 1024 || s->blocksize > 65536) return -1;
    s->blocks_per_group = rd32(sb + 0x20);
    s->inodes_per_group = rd32(sb + 0x28);
    s->first_data_block = rd32(sb + 0x14);
    s->inode_size = rd16(sb + 0x58);
    if (s->inode_size < 128 || s->inode_size > 256) s->inode_size = 128;
    s->desc_size = rd16(sb + 0xfe);
    if (s->desc_size == 0) s->desc_size = 32;
    s->inodes_count = rd32(sb + 0x00);
    s->blocks_count = rd32(sb + 0x04);
    if (rd32(sb + 0x60) & EXT4_FEATURE_INCOMPAT_64BIT)
        s->blocks_count |= (u64)rd32(sb + 0x150) << 32;
    s->free_blocks = rd32(sb + 0x0c);
    if (rd32(sb + 0x60) & EXT4_FEATURE_INCOMPAT_64BIT)
        s->free_blocks |= (u64)rd32(sb + 0x158) << 32;
    s->free_inodes = rd32(sb + 0x10);
    s->feature_compat = rd32(sb + 0x5c);
    s->feature_incompat = rd32(sb + 0x60);
    s->feature_ro_compat = rd32(sb + 0x64);
    memcpy(s->uuid, sb + 0x68, 16);
    s->checksum = rd32(sb + 0x3fc);

    s->want_extra_isize = rd16(sb + 0x15e);
    if (s->want_extra_isize == 0 && s->inode_size > 128)
        s->want_extra_isize = 32;
    if (s->want_extra_isize > s->inode_size - 128)
        s->want_extra_isize = s->inode_size - 128;

    s->has_csum = !!(s->feature_ro_compat & EXT4_FEATURE_RO_COMPAT_METADATA_CSUM);
    s->has_extents = !!(s->feature_incompat & EXT4_FEATURE_INCOMPAT_EXTENTS);
    s->has_64bit = !!(s->feature_incompat & EXT4_FEATURE_INCOMPAT_64BIT);
    s->has_flex_bg = !!(s->feature_incompat & EXT4_FEATURE_INCOMPAT_FLEX_BG);
    s->has_dir_index = !!(s->feature_compat & EXT4_FEATURE_COMPAT_DIR_INDEX);
    s->has_filetype = !!(s->feature_incompat & EXT4_FEATURE_INCOMPAT_FILETYPE);

    s->num_groups = (s->blocks_count + s->blocks_per_group - 1) / s->blocks_per_group;
    if (s->num_groups == 0) return -1;

    /* group descriptor table offset (independent of first_data_block) */
    gdt_off = ((2048 + s->blocksize - 1) / s->blocksize) * s->blocksize;
    s->gdt_block = gdt_off / s->blocksize;
    s->gdt_size = s->num_groups * s->desc_size;

    /* base checksum seed */
    if (s->has_csum)
        s->csum_seed = ext4_crc32c(s->uuid, 0xFFFFFFFF, 16);
    else
        s->csum_seed = 0;

    return 0;
}

/* recompute and store the superblock checksum (sbuf is the raw copy) */
static void ext4_sb_set_csum(ext4_dev *d)
{
    ext4_sb *s = &d->sb;
    u8 *sb = d->sbuf;
    if (!s->has_csum) return;
    wr32(sb + 0x3fc, ext4_crc32c(sb, 0xFFFFFFFF, 0x3fc));
}

/* persist superblock free counters + checksum */
static void ext4_write_super(ext4_dev *d, int id)
{
    ext4_sb *s = &d->sb;
    u8 *sb = d->sbuf;
    wr32(sb + 0x0c, (u32)(s->free_blocks & 0xFFFFFFFF));
    if (s->has_64bit)
        wr32(sb + 0x158, (u32)(s->free_blocks >> 32));
    wr32(sb + 0x10, s->free_inodes);
    ext4_sb_set_csum(d);
    {
        DWORD h = dex32_requestIO(id, IO_WRITE, 2, 2, sb);
        ext4_wait(h);
        dex32_closeIO(h);
    }
}

/* ------------------------------------------------------------------ */
/* group descriptor table                                              */
/* ------------------------------------------------------------------ */
static int ext4_load_gdt(ext4_dev *d, int id)
{
    ext4_sb *s = &d->sb;
    u32 nbytes = s->gdt_size;
    u32 blocks = (nbytes + s->blocksize - 1) / s->blocksize;
    u32 g;

    d->gdt_raw = (u8 *)malloc(nbytes);
    if (!d->gdt_raw) return -1;
    d->gdt = (ext4_gd *)malloc(s->num_groups * sizeof(ext4_gd));
    if (!d->gdt) { free(d->gdt_raw); d->gdt_raw = 0; return -1; }

    if (ext4_read_blocks(d, id, s->gdt_block, blocks, d->gdt_raw) != 0) return -1;

    for (g = 0; g < s->num_groups; g++)
    {
        u8 *desc = d->gdt_raw + g * s->desc_size;
        ext4_gd *gd = &d->gdt[g];
        gd->bg_block_bitmap = rd32(desc + 0x00);
        gd->bg_inode_bitmap = rd32(desc + 0x04);
        gd->bg_inode_table = rd32(desc + 0x08);
        gd->bg_free_blocks = rd16(desc + 0x0c);
        gd->bg_free_inodes = rd16(desc + 0x0e);
        gd->bg_used_dirs = rd16(desc + 0x10);
        gd->bg_flags = rd16(desc + 0x12);
        gd->bg_itable_unused = rd16(desc + 0x1c);
        gd->bg_checksum = rd16(desc + 0x1e);
        if (s->desc_size >= 64) {
            gd->bg_block_bitmap_hi = rd32(desc + 0x20);
            gd->bg_inode_bitmap_hi = rd32(desc + 0x24);
            gd->bg_inode_table_hi = rd32(desc + 0x28);
            gd->bg_free_blocks = (u16)(gd->bg_free_blocks | (rd16(desc + 0x2c) << 16));
            gd->bg_free_inodes = (u16)(gd->bg_free_inodes | (rd16(desc + 0x2e) << 16));
            gd->bg_used_dirs = (u16)(gd->bg_used_dirs | (rd16(desc + 0x30) << 16));
            gd->bg_itable_unused = (u16)(gd->bg_itable_unused | (rd16(desc + 0x32) << 16));
        }
    }
    return 0;
}

static u32 ext4_gd_blockno(const ext4_gd *gd, u32 lo)
{
    if (gd->bg_block_bitmap_hi) return lo; /* 64-bit: low field holds low bits */
    return lo;
}

/* recompute a descriptor checksum and store it in the raw descriptor */
static void ext4_gd_set_csum(ext4_dev *d, u32 g)
{
    ext4_sb *s = &d->sb;
    u8 *desc = d->gdt_raw + g * s->desc_size;
    u8 tmp[64];
    u8 gno[4];
    u32 c;
    if (!s->has_csum) return;
    le32buf(gno, g);
    c = ext4_crc32c(gno, s->csum_seed, 4);
    memcpy(tmp, desc, s->desc_size);
    tmp[0x1e] = 0; tmp[0x1f] = 0;
    c = ext4_crc32c(tmp, c, s->desc_size);
    wr16(desc + 0x1e, (u16)(c & 0xFFFF));
}

/* persist the whole GDT */
static void ext4_write_gdt(ext4_dev *d, int id)
{
    ext4_sb *s = &d->sb;
    u32 blocks = (s->gdt_size + s->blocksize - 1) / s->blocksize;
    ext4_write_blocks(d, id, s->gdt_block, blocks, d->gdt_raw);
}

/* ------------------------------------------------------------------ */
/* inode read / write                                                  */
/* ------------------------------------------------------------------ */
static u32 ext4_inode_block(ext4_dev *d, u32 ino)
{
    ext4_sb *s = &d->sb;
    u32 idx = ino - 1;
    u32 g = idx / s->inodes_per_group;
    u32 ing = idx % s->inodes_per_group;
    ext4_gd *gd = &d->gdt[g];
    /* byte offset: inode_table is an ext4 block number */
    return (u32)((u64)gd->bg_inode_table * s->blocksize +
                 (u64)ing * s->inode_size);
}

static int ext4_read_ino(ext4_dev *d, int id, u32 ino, u8 *raw)
{
    u32 blk = ext4_inode_block(d, ino);
    u32 off = (blk % d->sb.blocksize);
    u8 buf[4096];
    /* inode table block may not align to a single ext4 block when
       inode_size != blocksize, so read the containing block and copy */
    u32 blkno = blk / d->sb.blocksize;
    (void)off;
    if (ext4_read_blocks(d, id, blkno, 1, buf) != 0) return -1;
    memcpy(raw, buf + (blk % d->sb.blocksize), d->sb.inode_size);
    return 0;
}

static void ext4_ino_set_csum(ext4_dev *d, u32 ino, u32 gen, u8 *raw)
{
    ext4_sb *s = &d->sb;
    u8 seedbuf[8];
    u8 zero2[2];
    u32 c;
    if (!s->has_csum) return;
    le32buf(seedbuf, ino);
    le32buf(seedbuf + 4, gen);
    c = ext4_crc32c(seedbuf, s->csum_seed, 8);
    c = ext4_crc32c(raw, c, 0x7c);
    zero2[0] = 0; zero2[1] = 0;
    c = ext4_crc32c(zero2, c, 2);
    c = ext4_crc32c(raw + 0x7e, c, 2);
    if (s->inode_size > 128) {
        c = ext4_crc32c(raw + 0x80, c, 2);
        c = ext4_crc32c(zero2, c, 2);
        c = ext4_crc32c(raw + 0x84, c, s->inode_size - 0x84);
    }
    wr16(raw + 0x7c, (u16)(c & 0xFFFF));
    wr16(raw + 0x82, (u16)((c >> 16) & 0xFFFF));
}

static int ext4_write_ino(ext4_dev *d, int id, u32 ino, u32 gen, u8 *raw)
{
    u32 blk = ext4_inode_block(d, ino);
    u8 buf[4096];
    u32 blkno = blk / d->sb.blocksize;
    if (ext4_read_blocks(d, id, blkno, 1, buf) != 0) return -1;
    memcpy(buf + (blk % d->sb.blocksize), raw, d->sb.inode_size);
    if (ext4_write_blocks(d, id, blkno, 1, buf) != 0) return -1;
    return 0;
}

/* ------------------------------------------------------------------ */
/* extent tree                                                         */
/* ------------------------------------------------------------------ */

/* process one extent node (read from block), append leaf extents to
   runs. returns 0 ok, -1 error */
static int ext4_flatten_node(ext4_dev *d, int id, u32 phys, u16 depth,
                             ext4_run *runs, u32 *nruns)
{
    u8 blkbuf[4096];
    ext4_extent_header *hdr;
    u32 n, i;
    if (ext4_read_blocks(d, id, phys, 1, blkbuf) != 0) return -1;
    hdr = (ext4_extent_header *)blkbuf;
    if (hdr->eh_magic != EXT4_EXTENT_MAGIC) return -1;
    n = hdr->eh_entries;
    if (depth == 0)
    {
        ext4_extent *ext = (ext4_extent *)(blkbuf + 12);
        for (i = 0; i < n; i++)
        {
            u32 physb = ((u32)ext[i].ee_start_hi << 32) | ext[i].ee_start_lo;
            if (*nruns >= EXT4_MAX_RUNS) return -1;
            runs[*nruns].log_block = ext[i].ee_block;
            runs[*nruns].len = ext[i].ee_len;
            runs[*nruns].phys_lo = (u32)(physb & 0xFFFFFFFF);
            runs[*nruns].phys_hi = (u32)(physb >> 32);
            (*nruns)++;
        }
    }
    else
    {
        ext4_extent_idx *idx = (ext4_extent_idx *)(blkbuf + 12);
        for (i = 0; i < n; i++)
        {
            u32 leaf = ((u32)idx[i].ei_leaf_hi << 32) | idx[i].ei_leaf_lo;
            if (ext4_flatten_node(d, id, leaf, (u16)(depth - 1), runs, nruns) < 0)
                return -1;
        }
    }
    return 0;
}

/* build the run list from an inode's extent root (stored in i_block).
   raw must be the raw inode buffer. */
static int ext4_build_runs(ext4_dev *d, int id, u8 *raw, ext4_run *runs, u32 *nruns)
{
    ext4_extent_header *hdr = (ext4_extent_header *)(raw + 0x28);
    *nruns = 0;
    if (hdr->eh_magic != EXT4_EXTENT_MAGIC) return 0; /* no extents */
    if (hdr->eh_depth == 0)
    {
        ext4_extent *ext = (ext4_extent *)(raw + 0x28 + 12);
        u32 i;
        for (i = 0; i < hdr->eh_entries; i++)
        {
            u32 physb = ((u32)ext[i].ee_start_hi << 32) | ext[i].ee_start_lo;
            if (*nruns >= EXT4_MAX_RUNS) return -1;
            runs[*nruns].log_block = ext[i].ee_block;
            runs[*nruns].len = ext[i].ee_len;
            runs[*nruns].phys_lo = (u32)(physb & 0xFFFFFFFF);
            runs[*nruns].phys_hi = (u32)(physb >> 32);
            (*nruns)++;
        }
    }
    else
    {
        ext4_extent_idx *idx = (ext4_extent_idx *)(raw + 0x28 + 12);
        u32 i;
        for (i = 0; i < hdr->eh_entries; i++)
        {
            u32 leaf = ((u32)idx[i].ei_leaf_hi << 32) | idx[i].ei_leaf_lo;
            if (ext4_flatten_node(d, id, leaf, (u16)(hdr->eh_depth - 1), runs, nruns) < 0)
                return -1;
        }
    }
    return 0;
}

/* write an external extent leaf node (block) for a set of runs */
static int ext4_write_extent_leaf(ext4_dev *d, int id, u32 phys,
                                   ext4_run *runs, u32 start, u32 count,
                                   u32 gen, u32 ino)
 {
    ext4_sb *s = &d->sb;
    u8 blkbuf[4096];
    ext4_extent_header *hdr;
    u32 i, max_entries;
    max_entries = (s->blocksize - 12) / 12;
    memset(blkbuf, 0, s->blocksize);
    hdr = (ext4_extent_header *)blkbuf;
    hdr->eh_magic = EXT4_EXTENT_MAGIC;
    hdr->eh_entries = (u16)count;
    hdr->eh_max = (u16)max_entries;
    hdr->eh_depth = 0;
    hdr->eh_unused = 0;
    hdr->eh_generation = gen;
    for (i = 0; i < count; i++)
    {
        ext4_extent *ext = (ext4_extent *)(blkbuf + 12 + i * 12);
        u32 physb = ((u32)runs[start + i].phys_hi << 32) | runs[start + i].phys_lo;
        ext->ee_block = runs[start + i].log_block;
        ext->ee_len = runs[start + i].len;
        ext->ee_start_hi = (u16)(physb >> 32);
        ext->ee_start_lo = (u32)(physb & 0xFFFFFFFF);
    }
    if (s->has_csum)
    {
        u32 c;
        u8 seedbuf[8];
        /* leaf checksum uses the per-inode seed; feed header+entries,
           store after the 2-byte et_padding tail */
        le32buf(seedbuf, ino);
        le32buf(seedbuf + 4, gen);
        c = ext4_crc32c(seedbuf, s->csum_seed, 8);
        c = ext4_crc32c(blkbuf, c, 12 + max_entries * 12 + 2);
        wr32(blkbuf + 12 + max_entries * 12 + 2, c);
    }
    if (ext4_write_blocks(d, id, phys, 1, blkbuf) != 0) return -1;
    return 0;
}

/* free previously allocated extent-node blocks */
static void ext4_free_extent_leaves(ext4_dev *d, int id, ext4_ino *c)
{
    u32 i;
    for (i = 0; i < c->nleaf; i++)
        ext4_free_block_raw(d, id, c->leaf_blocks[i]);
    c->nleaf = 0;
}

/* rebuild + persist the extent tree for the cache. raw = c->inode_raw. */
static int ext4_write_extents(ext4_dev *d, int id, ext4_ino *c)
{
    ext4_sb *s = &d->sb;
    u8 *raw = c->inode_raw;
    u32 nruns = c->nruns;
    u32 per_leaf;
    u32 i;

    per_leaf = (s->blocksize - 12) / 12;
    if (per_leaf < 1) per_leaf = 1;

    /* release old external nodes before rebuilding */
    if (c->nleaf)
    {
        u32 i2;
        for (i2 = 0; i2 < c->nleaf; i2++)
            ext4_free_block_raw(d, id, c->leaf_blocks[i2]);
        c->nleaf = 0;
    }

    memset(raw + 0x28, 0, 60); /* clear i_block */

    if (nruns == 0)
        return 0;

    if (nruns <= 4)
    {
        ext4_extent_header *hdr = (ext4_extent_header *)(raw + 0x28);
        hdr->eh_magic = EXT4_EXTENT_MAGIC;
        hdr->eh_entries = (u16)nruns;
        hdr->eh_max = EXT4_INLINE_EXTENT_MAX;
        hdr->eh_depth = 0;
        hdr->eh_unused = 0;
        hdr->eh_generation = c->generation;
        for (i = 0; i < nruns; i++)
        {
            ext4_extent *ext = (ext4_extent *)(raw + 0x28 + 12 + i * 12);
            u32 physb = ((u32)c->runs[i].phys_hi << 32) | c->runs[i].phys_lo;
            ext->ee_block = c->runs[i].log_block;
            ext->ee_len = c->runs[i].len;
            ext->ee_start_hi = (u16)(physb >> 32);
            ext->ee_start_lo = (u32)(physb & 0xFFFFFFFF);
        }
        return 0;
    }

    /* two-level tree: root idx (in i_block) -> leaf nodes */
    {
        u32 nleaf = (nruns + per_leaf - 1) / per_leaf;
        ext4_extent_header *hdr = (ext4_extent_header *)(raw + 0x28);
        if (nleaf > 4) return -1; /* would need 3-level; not handled */
        hdr->eh_magic = EXT4_EXTENT_MAGIC;
        hdr->eh_entries = (u16)nleaf;
        hdr->eh_max = EXT4_INLINE_EXTENT_MAX;
        hdr->eh_depth = 1;
        hdr->eh_unused = 0;
        hdr->eh_generation = c->generation;
        for (i = 0; i < nleaf; i++)
        {
            u32 cnt = per_leaf;
            u32 start = i * per_leaf;
            u32 leafblk;
            ext4_extent_idx *idx;
            if (start + cnt > nruns) cnt = nruns - start;
            leafblk = ext4_alloc_block_raw(d, id);
            if (leafblk == (u32)-1) return -1;
 if (ext4_write_extent_leaf(d, id, leafblk, c->runs, start, cnt,
                                        c->generation, c->ino) < 0)
                return -1;
            if (c->nleaf < EXT4_MAX_LEAVES)
                c->leaf_blocks[c->nleaf++] = leafblk;
            idx = (ext4_extent_idx *)(raw + 0x28 + 12 + i * 12);
            idx->ei_block = 0;
            idx->ei_leaf_lo = (u32)(leafblk & 0xFFFFFFFF);
            idx->ei_leaf_hi = (u16)(leafblk >> 32);
            idx->ei_unused = 0;
        }
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* block + inode allocation                                            */
/* ------------------------------------------------------------------ */

static void ext4_pad_block_bitmap(ext4_dev *d, u8 *bitmap, u32 g)
{
    ext4_sb *s = &d->sb;
    u32 total = s->blocks_per_group;
    if (g == s->num_groups - 1)
    {
        u64 last = s->blocks_count - (u64)g * s->blocks_per_group;
        if (last < total) total = (u32)last;
    }
    {
        u32 full = total / 8;
        u32 rem = total % 8;
        u32 i;
        if (rem) bitmap[full] |= (u8)(0xFF << rem);
        for (i = full + (rem ? 1 : 0); i < s->blocksize; i++)
            bitmap[i] = 0xFF;
    }
}

static void ext4_pad_inode_bitmap(ext4_dev *d, u8 *bitmap, u32 g)
{
    ext4_sb *s = &d->sb;
    u32 total = s->inodes_per_group;
    if (g == s->num_groups - 1)
    {
        u32 last = s->inodes_count - g * s->inodes_per_group;
        if (last < total) total = last;
    }
    {
        u32 full = total / 8;
        u32 rem = total % 8;
        u32 i;
        if (rem) bitmap[full] |= (u8)(0xFF << rem);
        for (i = full + (rem ? 1 : 0); i < s->blocksize; i++)
            bitmap[i] = 0xFF;
    }
}

/* mark a block used in its group bitmap and persist the bitmap block */
static void ext4_set_block_used(ext4_dev *d, int id, u32 block)
{
    ext4_sb *s = &d->sb;
    u32 g = block / s->blocks_per_group;
    u32 bit = block % s->blocks_per_group;
    u8 bitmap[4096];
    ext4_gd *gd = &d->gdt[g];
    u32 bb = gd->bg_block_bitmap;
    u32 bcsum = 0;
    if (bb == 0) return;
    if (ext4_read_blocks(d, id, bb, 1, bitmap) != 0) return;
    bitmap[bit / 8] |= (u8)(1 << (bit % 8));
    ext4_pad_block_bitmap(d, bitmap, g);
    if (s->has_csum)
        bcsum = ext4_crc32c(bitmap, s->csum_seed, s->blocksize);
    ext4_write_blocks(d, id, bb, 1, bitmap);
    gd->bg_free_blocks = (u16)(gd->bg_free_blocks - 1);
    /* update raw descriptor */
    {
        u8 *desc = d->gdt_raw + g * s->desc_size;
        if (s->desc_size >= 64) {
            wr16(desc + 0x0c, (u16)(gd->bg_free_blocks & 0xFFFF));
            wr16(desc + 0x2c, (u16)((gd->bg_free_blocks >> 16) & 0xFFFF));
        } else {
            wr16(desc + 0x0c, gd->bg_free_blocks);
        }
        if (s->has_csum) {
            wr16(desc + 0x18, (u16)(bcsum & 0xFFFF));
            if (s->desc_size >= 64)
                wr16(desc + 0x38, (u16)((bcsum >> 16) & 0xFFFF));
        }
        ext4_gd_set_csum(d, g);
    }
    s->free_blocks = (s->free_blocks > 0) ? s->free_blocks - 1 : 0;
}

/* raw: mark a block used, no superblock write (caller batches) */
static void ext4_free_block_raw(ext4_dev *d, int id, u32 block)
{
    ext4_sb *s = &d->sb;
    u32 g = block / s->blocks_per_group;
    u32 bit = block % s->blocks_per_group;
    u8 bitmap[4096];
    ext4_gd *gd = &d->gdt[g];
    u32 bb = gd->bg_block_bitmap;
    u32 bcsum = 0;
    if (bb == 0) return;
    if (ext4_read_blocks(d, id, bb, 1, bitmap) != 0) return;
    bitmap[bit / 8] &= (u8)~(1 << (bit % 8));
    ext4_pad_block_bitmap(d, bitmap, g);
    if (s->has_csum)
        bcsum = ext4_crc32c(bitmap, s->csum_seed, s->blocksize);
    ext4_write_blocks(d, id, bb, 1, bitmap);
    gd->bg_free_blocks = (u16)(gd->bg_free_blocks + 1);
    {
        u8 *desc = d->gdt_raw + g * s->desc_size;
        if (s->desc_size >= 64) {
            wr16(desc + 0x0c, (u16)(gd->bg_free_blocks & 0xFFFF));
            wr16(desc + 0x2c, (u16)((gd->bg_free_blocks >> 16) & 0xFFFF));
        } else {
            wr16(desc + 0x0c, gd->bg_free_blocks);
        }
        if (s->has_csum) {
            wr16(desc + 0x18, (u16)(bcsum & 0xFFFF));
            if (s->desc_size >= 64)
                wr16(desc + 0x38, (u16)((bcsum >> 16) & 0xFFFF));
        }
        ext4_gd_set_csum(d, g);
    }
    s->free_blocks++;
}

static u32 ext4_alloc_block_raw(ext4_dev *d, int id)
{
    ext4_sb *s = &d->sb;
    u32 g, b, total;
    u8 bitmap[4096];
    u32 hint_group = 0;
    u32 hint_bit = 0;

    if (d->alloc_block_hint)
    {
        hint_group = d->alloc_block_hint / s->blocks_per_group;
        if (hint_group >= s->num_groups) hint_group = 0;
        hint_bit = d->alloc_block_hint % s->blocks_per_group;
    }

    for (g = 0; g < s->num_groups; g++)
    {
        u32 gg = (g + hint_group) % s->num_groups;
        ext4_gd *gd = &d->gdt[gg];
        u32 bpg = s->blocks_per_group;
        u32 bb = gd->bg_block_bitmap;
        u32 start_bit, off;
        if (bb == 0) continue;
        total = bpg;
        if (gg == s->num_groups - 1)
        {
            u32 last = s->blocks_count - gg * bpg;
            if (last < total) total = last;
        }
        start_bit = (gg == hint_group) ? hint_bit : 0;
        if (start_bit >= total) start_bit = 0;
        if (ext4_read_blocks(d, id, bb, 1, bitmap) != 0) continue;
        for (off = 0; off < total; off++)
        {
            u32 bit = (start_bit + off) % total;
            if (!(bitmap[bit / 8] & (1 << (bit % 8))))
            {
                u32 block = gg * bpg + bit;
                ext4_set_block_used(d, id, block);
                d->alloc_block_hint = block + 1;
                return block;
            }
        }
    }
    return (u32)-1;
}

/* mark an inode used in its group inode bitmap and persist */
static void ext4_set_ino_used(ext4_dev *d, int id, u32 ino)
{
    ext4_sb *s = &d->sb;
    u32 idx = ino - 1;
    u32 g = idx / s->inodes_per_group;
    u32 bit = idx % s->inodes_per_group;
    u8 bitmap[4096];
    ext4_gd *gd = &d->gdt[g];
    u32 ib = gd->bg_inode_bitmap;
    u32 icsum = 0;
    if (ib == 0) return;
    if (ext4_read_blocks(d, id, ib, 1, bitmap) != 0) return;
    bitmap[bit / 8] |= (u8)(1 << (bit % 8));
    ext4_pad_inode_bitmap(d, bitmap, g);
    if (s->has_csum)
        icsum = ext4_crc32c(bitmap, s->csum_seed, (s->inodes_per_group + 7) / 8);
    ext4_write_blocks(d, id, ib, 1, bitmap);
    gd->bg_free_inodes = (u16)(gd->bg_free_inodes - 1);
    gd->bg_itable_unused = (u16)(gd->bg_itable_unused - 1);
    {
        u8 *desc = d->gdt_raw + g * s->desc_size;
        if (s->desc_size >= 64) {
            wr16(desc + 0x0e, (u16)(gd->bg_free_inodes & 0xFFFF));
            wr16(desc + 0x2e, (u16)((gd->bg_free_inodes >> 16) & 0xFFFF));
            wr16(desc + 0x1c, (u16)(gd->bg_itable_unused & 0xFFFF));
            wr16(desc + 0x32, (u16)((gd->bg_itable_unused >> 16) & 0xFFFF));
        } else {
            wr16(desc + 0x0e, gd->bg_free_inodes);
            wr16(desc + 0x1c, gd->bg_itable_unused);
        }
        if (s->has_csum) {
            wr16(desc + 0x1a, (u16)(icsum & 0xFFFF));
            if (s->desc_size >= 64)
                wr16(desc + 0x3a, (u16)((icsum >> 16) & 0xFFFF));
        }
        ext4_gd_set_csum(d, g);
    }
    s->free_inodes = (s->free_inodes > 0) ? s->free_inodes - 1 : 0;
}

static u32 ext4_alloc_ino(ext4_dev *d, int id)
{
    ext4_sb *s = &d->sb;
    u32 g, b, total;
    u8 bitmap[4096];

    for (g = 0; g < s->num_groups; g++)
    {
        ext4_gd *gd = &d->gdt[g];
        u32 ib = gd->bg_inode_bitmap;
        u32 start_bit = 0;
        u32 off;
        if (ib == 0) continue;
        total = s->inodes_per_group;
        if (g == s->num_groups - 1)
        {
            u32 inodes_total = s->inodes_per_group * s->num_groups;
            u32 last = inodes_total - g * s->inodes_per_group;
            if (last < total) total = last;
        }
        if (d->alloc_ino_hint && (d->alloc_ino_hint - 1) / s->inodes_per_group == g)
            start_bit = (d->alloc_ino_hint - 1) % s->inodes_per_group;
        if (start_bit >= total) start_bit = 0;
        if (ext4_read_blocks(d, id, ib, 1, bitmap) != 0) continue;
        for (off = 0; off < total; off++)
        {
            u32 bit = (start_bit + off) % total;
            if (!(bitmap[bit / 8] & (1 << (bit % 8))))
            {
                u32 ino = g * s->inodes_per_group + bit + 1;
                ext4_set_ino_used(d, id, ino);
                d->alloc_ino_hint = ino + 1;
                return ino;
            }
        }
    }
    return (u32)-1;
}

/* clear an inode, free it in its group inode bitmap, update counters */
static int ext4_free_ino_raw(ext4_dev *d, int id, u32 ino)
{
    ext4_sb *s = &d->sb;
    u32 idx = ino - 1;
    u32 g = idx / s->inodes_per_group;
    u32 bit = idx % s->inodes_per_group;
    u32 ing = idx % s->inodes_per_group;
    u8 bitmap[4096];
    u8 raw[256];
    ext4_gd *gd = &d->gdt[g];
    u32 ib = gd->bg_inode_bitmap;
    u32 icsum = 0;
    if (g >= s->num_groups) return -1;
    /* clear the on-disk inode */
    memset(raw, 0, s->inode_size);
    if (ext4_write_ino(d, id, ino, 0, raw) < 0) return -1;
    /* clear the inode bitmap bit */
    if (ib != 0)
    {
        if (ext4_read_blocks(d, id, ib, 1, bitmap) != 0) return -1;
        bitmap[bit / 8] &= (u8)~(1 << (bit % 8));
        ext4_pad_inode_bitmap(d, bitmap, g);
        if (s->has_csum)
            icsum = ext4_crc32c(bitmap, s->csum_seed, (s->inodes_per_group + 7) / 8);
        ext4_write_blocks(d, id, ib, 1, bitmap);
    }
    gd->bg_free_inodes = (u16)(gd->bg_free_inodes + 1);
    gd->bg_itable_unused = (u16)(gd->bg_itable_unused + 1);
    {
        u8 *desc = d->gdt_raw + g * s->desc_size;
        if (s->desc_size >= 64) {
            wr16(desc + 0x0e, (u16)(gd->bg_free_inodes & 0xFFFF));
            wr16(desc + 0x2e, (u16)((gd->bg_free_inodes >> 16) & 0xFFFF));
            wr16(desc + 0x1c, (u16)(gd->bg_itable_unused & 0xFFFF));
            wr16(desc + 0x32, (u16)((gd->bg_itable_unused >> 16) & 0xFFFF));
        } else {
            wr16(desc + 0x0e, gd->bg_free_inodes);
            wr16(desc + 0x1c, gd->bg_itable_unused);
        }
        if (s->has_csum) {
            wr16(desc + 0x1a, (u16)(icsum & 0xFFFF));
            if (s->desc_size >= 64)
                wr16(desc + 0x3a, (u16)((icsum >> 16) & 0xFFFF));
        }
        ext4_gd_set_csum(d, g);
    }
    s->free_inodes++;
    (void)ing;
    return 0;
}

/* ------------------------------------------------------------------ */
/* directory image helpers                                             */
/* ------------------------------------------------------------------ */

/* load the directory blocks for inode `ino` into a freshly malloc'd
   buffer; returns byte count and sets *nblocks. returns 0 on failure */
static int ext4_load_dirimage(ext4_dev *d, int id, u32 ino, u8 **out,
                              u32 *out_bytes, u32 *out_nblocks)
{
    u8 raw[256];
    ext4_run runs[EXT4_MAX_RUNS];
    u32 nruns = 0, i;
    u32 nblocks = 0;
    u8 *img;
    u64 size;
    if (ext4_read_ino(d, id, ino, raw) < 0) return -1;
    size = ((u64)rd32(raw + 0x0c) << 32) | rd32(raw + 0x04);
    if (size == 0)
    {
        *out = 0; *out_bytes = 0; *out_nblocks = 0;
        return 0;
    }
    if (ext4_build_runs(d, id, raw, runs, &nruns) < 0) return -1;
    for (i = 0; i < nruns; i++)
        nblocks += runs[i].len;
    if (nblocks == 0)
    {
        *out = 0; *out_bytes = 0; *out_nblocks = 0;
        return 0;
    }
    img = (u8 *)malloc((u32)(nblocks * d->sb.blocksize));
    if (!img) return -1;
    memset(img, 0, (u32)(nblocks * d->sb.blocksize));
    for (i = 0; i < nruns; i++)
    {
        u32 j;
        u32 phys = runs[i].phys_lo;
        u8 *dst = img + (u32)(runs[i].log_block * d->sb.blocksize);
        if (phys == 0 && runs[i].phys_hi == 0) continue; /* hole */
        for (j = 0; j < runs[i].len; j++)
        {
            if (ext4_read_blocks(d, id, (u64)(phys + j), 1, dst + j * d->sb.blocksize) < 0)
            {
                free(img);
                return -1;
            }
        }
    }
    *out = img;
    *out_bytes = (u32)(nblocks * d->sb.blocksize);
    *out_nblocks = nblocks;
    return 0;
}

/* write back a modified directory image (all blocks) */
static int ext4_write_dirimage(ext4_dev *d, int id, u32 ino, u8 *img, u32 nblocks)
{
    u8 raw[256];
    ext4_run runs[EXT4_MAX_RUNS];
    u32 nruns = 0, i;
    u32 gen;
    if (ext4_read_ino(d, id, ino, raw) < 0) return -1;
    gen = rd32(raw + 0x64);
    if (ext4_build_runs(d, id, raw, runs, &nruns) < 0) return -1;
    for (i = 0; i < nruns; i++)
    {
        u32 j;
        u32 phys = runs[i].phys_lo;
        u8 *src = img + (u32)(runs[i].log_block * d->sb.blocksize);
        if (phys == 0 && runs[i].phys_hi == 0) continue;
        for (j = 0; j < runs[i].len; j++)
        {
            u8 blkbuf[4096];
            memcpy(blkbuf, src + j * d->sb.blocksize, d->sb.blocksize);
            /* directory leaf checksum (per-inode seed) */
            if (d->sb.has_csum)
            {
                u8 seedbuf[8];
                u32 c;
                le32buf(seedbuf, ino);
                le32buf(seedbuf + 4, gen);
                c = ext4_crc32c(seedbuf, d->sb.csum_seed, 8);
                c = ext4_crc32c(blkbuf, c, d->sb.blocksize - 12);
                wr32(blkbuf + d->sb.blocksize - 4, c);
            }
            if (ext4_write_blocks(d, id, (u64)(phys + j), 1, blkbuf) < 0)
                return -1;
        }
    }
    return 0;
}

/* scan a directory image and create vfs child nodes. `parent` is the
   directory node being populated. returns 0 ok */
static int ext4_populate_dir(vfs_node *parent, u8 *img, u32 bytes, int id,
                              u32 ino)
{
    ext4_dev *d = ext4_get_dev(id);
    u32 bs = d->sb.blocksize;
    u32 nblocks = bytes / bs;
    u32 blk, boff;

    (void)ino;
    parent->files = 0;
    for (blk = 0; blk < nblocks; blk++)
    {
        u8 *base = img + blk * bs;
        for (boff = 0; boff + 8 <= bs;)
        {
            ext4_dirent *e = (ext4_dirent *)(base + boff);
            u16 rl = e->rec_len;
            vfs_node *child;
            if (rl < 8) break;
            if (e->inode == 0) break;  /* tail / end of this block */
            if (e->name_len == 0) { boff += rl; continue; }
            if (e->name[0] == '.' && e->name_len == 1) { boff += rl; continue; }
            if (e->name[0] == '.' && e->name[1] == '.' && e->name_len == 2) { boff += rl; continue; }

            child = (vfs_node *)malloc(sizeof(vfs_node));
            if (!child) break;
            memset(child, 0, sizeof(vfs_node));
            {
                u32 nl = e->name_len < 255 ? e->name_len : 255;
                memcpy(child->name, e->name, nl);
                child->name[nl] = 0;
            }
            child->fsid = parent->fsid;
            child->memid = id;
            child->size = 0;

            /* per-child ext4 cache */
            {
                ext4_ino *c = (ext4_ino *)malloc(sizeof(ext4_ino));
                if (!c) { free(child); boff += rl; continue; }
                memset(c, 0, sizeof(ext4_ino));
                c->ino = e->inode;
                c->is_dir = (e->file_type == EXT4_FT_DIR);
                c->dirent = (void *)e;
                c->dirent_valid = 1;
                child->misc = (void *)c;
                child->miscsize = sizeof(ext4_ino);
            }
            /* load the real on-disk size so the VFS read/write path
               bounds correctly (node->size is the source of truth). */
            {
                u8 iraw[256];
                if (ext4_read_ino(d, id, e->inode, iraw) == 0)
                {
                    ext4_ino *cc = (ext4_ino *)child->misc;
                    cc->size_lo = rd32(iraw + 0x04);
                    cc->size_hi = rd32(iraw + 0x6c);
                    child->size = (int)(((u64)cc->size_hi << 32) | cc->size_lo);
                }
            }
            if (e->file_type == EXT4_FT_DIR)
            {
                child->attb = FILE_DIRECTORY | FILE_OREAD | FILE_OWRITE;
                child->files = VFS_NOT_MOUNTED;
            }
            else
            {
                child->attb = FILE_OREAD | FILE_OWRITE;
                if (e->file_type == EXT4_FT_REG_FILE) child->attb |= FILE_OEXE;
                child->files = 0;
            }
            vfs_createnode(child, parent);
            boff += rl;
        }
    }
    return 0;
}

/* allocate + fill a directory entry in the parent image. Splits the
   rec_len of an existing entry that has room; never touches the
   12-byte tail (inode==0). Returns pointer to the new entry, or 0. */
static ext4_dirent *ext4_alloc_entry(ext4_dev *d, u8 *img, u32 *bytes,
                                     u32 ino, const char *name, u8 file_type)
{
    u32 bs = d->sb.blocksize;
    u32 nl = (u32)strlen(name);
    u32 need = (8 + nl + 3) & ~3u;
    u32 nblocks = *bytes / bs;
    u32 blk, boff;
    u32 seen;
    if (need > bs) return 0;
    seen = 0;
    for (blk = 0; blk < nblocks; blk++)
    {
        u8 *base = img + blk * bs;
        for (boff = 0; boff + 8 <= bs;)
        {
            ext4_dirent *e = (ext4_dirent *)(base + boff);
            u32 rl = e->rec_len;
            if (rl < 8) break;
            if (e->inode == 0) break;  /* stop at tail / free slot */
            if (blk == 0 && seen == 1)
            {
                /* `..` must remain the second entry; insert after it. */
                if (rl >= 12 + need)
                {
                    ext4_dirent *ne = (ext4_dirent *)(base + boff + 12);
                    e->rec_len = 12;
                    ne->inode = ino;
                    ne->rec_len = (u16)(rl - 12);
                    ne->name_len = (u8)nl;
                    ne->file_type = file_type;
                    memcpy(ne->name, name, nl);
                    return ne;
                }
            }
            else if (rl >= need + 8 + e->name_len)
            {
                u32 rem = rl - need;
                /* shift the existing entry (header + name) forward */
                memmove(base + boff + need, base + boff, 8 + e->name_len);
                ((ext4_dirent *)(base + boff + need))->rec_len = (u16)rem;
                /* fill the new entry */
                e->inode = ino;
                e->rec_len = (u16)need;
                e->name_len = (u8)nl;
                e->file_type = file_type;
                memcpy(e->name, name, nl);
                return e;
            }
            seen++;
            boff += rl;
        }
    }
    return 0;
}

static ext4_ino *ext4_get_cache(vfs_node *f);

/* write the parent directory image back to disk (recomputing per-block
   leaf checksums). */
static void ext4_write_parent_dir(ext4_dev *d, int id, vfs_node *parent,
                                  u8 *img, u32 bytes)
{
    ext4_ino *pc = ext4_get_cache(parent);
    u32 nblocks = bytes / d->sb.blocksize;
    if (pc)
        ext4_write_dirimage(d, id, pc->ino, img, nblocks);
}

/* after marking an entry's inode to 0, merge it into the previous entry
   (or leave it as a zeroed gap) and update the block. */
static void ext4_merge_delete(ext4_dev *d, int id, vfs_node *parent,
                              u8 *img, ext4_dirent *e)
{
    u32 bs = d->sb.blocksize;
    u32 eoff = (u32)((u8 *)e - img);
    u32 boff = eoff & ~(bs - 1);       /* offset of entry within its block */
    u8 *base = img + boff;
    u32 off, prev_off = 0;
    int have_prev = 0;
    (void)d;
    for (off = 0; off + 8 <= bs;)
    {
        ext4_dirent *p = (ext4_dirent *)(base + off);
        if ((u32)((u8 *)p - img) == eoff) break;
        if (p->inode == 0) break;
        prev_off = off;
        have_prev = 1;
        off += p->rec_len;
    }
    if (have_prev)
    {
        ext4_dirent *p = (ext4_dirent *)(base + prev_off);
        p->rec_len = (u16)(p->rec_len + e->rec_len);
    }
}

/* ------------------------------------------------------------------ */
/* node cache helpers                                                  */
/* ------------------------------------------------------------------ */

static ext4_ino *ext4_get_cache(vfs_node *f)
{
    return (ext4_ino *)f->misc;
}

/* ensure the inode + runs are loaded for the cache */
static int ext4_ensure(ext4_dev *d, int id, ext4_ino *c)
{
    u8 raw[256];
    u64 size;
    if (c->runs_loaded) return 0;
    if (ext4_read_ino(d, id, c->ino, raw) < 0) return -1;
    memcpy(c->inode_raw, raw, d->sb.inode_size);
    c->inode_loaded = 1;
    c->size_lo = rd32(raw + 0x04);
    c->size_hi = rd32(raw + 0x6c);
    c->mode = rd16(raw + 0x00);
    c->links = rd16(raw + 0x1a);
    c->nblocks = rd32(raw + 0x1c);
    c->flags = rd32(raw + 0x20);
    c->generation = rd32(raw + 0x64);
    c->atime = rd32(raw + 0x08);
    c->ctime = rd32(raw + 0x0c);
    c->mtime = rd32(raw + 0x10);
    c->is_dir = (rd16(raw + 0x00) & EXT4_S_IFMT) == EXT4_S_IFDIR;
    c->nruns = 0;
    if (ext4_build_runs(d, id, raw, c->runs, &c->nruns) < 0) return -1;
    c->runs_loaded = 1;
    (void)size;
    return 0;
}

static int ext4_flush_ino(ext4_dev *d, int id, ext4_ino *c)
{
    u8 *raw = c->inode_raw;
    if (!c->inode_loaded) return -1;
    if (d->sb.has_csum)
        ext4_ino_set_csum(d, c->ino, c->generation, raw);
    if (ext4_write_ino(d, id, c->ino, c->generation, raw) < 0) return -1;
    return 0;
}

/* map a logical block to a physical block; 0 ok, -1 hole */
static int ext4_map_block(ext4_ino *c, u32 lblock, u64 *phys)
{
    u32 i;
    for (i = 0; i < c->nruns; i++)
    {
        if (lblock >= c->runs[i].log_block &&
            lblock < c->runs[i].log_block + c->runs[i].len)
        {
            *phys = ((u64)c->runs[i].phys_hi << 32) | c->runs[i].phys_lo;
            *phys += (lblock - c->runs[i].log_block);
            return 0;
        }
        if (lblock < c->runs[i].log_block) break;
    }
    return -1;
}

/* ------------------------------------------------------------------ */
/* devmgr callbacks                                                    */
/* ------------------------------------------------------------------ */

int ext4_identify(int device)
{
    u8 buf[512];
    DWORD h = dex32_requestIO(device, IO_READ, 2, 1, buf);
    /* superblock is at byte 1024 = sector 2 */
    ext4_wait(h);
    dex32_closeIO(h);
    if (rd16(buf + 0x38) == EXT4_SUPER_MAGIC) return 1;
    return 0;
}

static int ext4_mount_device_common(ext4_dev *d, int id)
{
    if (ext4_load_super(d, id) < 0) return -1;
    if (ext4_load_gdt(d, id) < 0) return -1;
    d->mounted = 1;
    return 0;
}

static int ext4_mountroot(vfs_node *f, int id)
{
    ext4_dev *d;
    u8 raw[256];
    u8 *img = 0;
    u32 bytes = 0, nblocks = 0;
    u32 rootino = 2;

    d = ext4_get_dev(id);
    if (!d)
    {
        d = (ext4_dev *)malloc(sizeof(ext4_dev));
        if (!d) return -1;
        memset(d, 0, sizeof(ext4_dev));
        d->devid = id;
        ext4_devs[id] = d;
    }
    if (!d->mounted)
    {
        if (ext4_mount_device_common(d, id) < 0)
        {
            return -1;
        }
    }

    /* root inode is 2 */
    if (ext4_read_ino(d, id, rootino, raw) < 0)
    {
        return -1;
    }

    /* per-mountpoint cache */
    {
        ext4_ino *c = (ext4_ino *)malloc(sizeof(ext4_ino));
        if (!c) return -1;
        memset(c, 0, sizeof(ext4_ino));
        c->ino = rootino;
        c->is_dir = 1;
        memcpy(c->inode_raw, raw, d->sb.inode_size);
        c->inode_loaded = 1;
        c->size_lo = rd32(raw + 0x04);
        c->size_hi = rd32(raw + 0x6c);
        c->nblocks = rd32(raw + 0x1c);
        c->mode = rd16(raw + 0x00);
        c->links = rd16(raw + 0x1a);
        c->flags = rd32(raw + 0x20);
        c->generation = rd32(raw + 0x64);
        if (ext4_build_runs(d, id, raw, c->runs, &c->nruns) < 0)
        {
            free(c);
            return -1;
        }
        c->runs_loaded = 1;
        f->misc = (void *)c;
        f->miscsize = sizeof(ext4_ino);
    }

    f->size = 0;
    f->attb = f->attb | FILE_OREAD | FILE_OWRITE;
    f->start_sector = 0;
    f->date_created.year = 2020;

    /* load root dir image */
    if (ext4_load_dirimage(d, id, rootino, &img, &bytes, &nblocks) < 0)
    {
        return -1;
    }
    if (bytes)
    {
        f->misc2 = (void *)img;
        f->miscsize2 = bytes;
    }
    ext4_populate_dir(f, img ? img : (u8 *)"", bytes, id, rootino);
    return 0;
}

static int ext4_mountdirectory(vfs_node *f, int id)
{
    ext4_dev *d = ext4_get_dev(id);
    ext4_ino *c;
    u8 *img = 0;
    u32 bytes = 0, nblocks = 0;
    if (!d) return -1;
    c = ext4_get_cache(f);
    if (!c) return -1;
    if (f->misc2)
    {
        /* already mounted */
        return 0;
    }
    if (ext4_load_dirimage(d, id, c->ino, &img, &bytes, &nblocks) < 0) return -1;
    if (bytes)
    {
        f->misc2 = (void *)img;
        f->miscsize2 = bytes;
        ext4_populate_dir(f, img, bytes, id, c->ino);
    }
    return 0;
}

static int ext4_readfile(vfs_node *f, char *buf, int start, int end, int id)
{
    ext4_dev *d = ext4_get_dev(id);
    ext4_ino *c;
    u64 fsize;
    u32 bs, startblk, endblk, startoff, endoff, ofs, lb;
    u8 *tmp;
    if (!d) return 0;
    c = ext4_get_cache(f);
    if (!c) return 0;
    if (ext4_ensure(d, id, c) < 0) return 0;
    fsize = ((u64)c->size_hi << 32) | c->size_lo;
    if (start < 0) start = 0;
    if ((u64)end >= fsize) end = (int)(fsize ? fsize - 1 : 0);
    if (start > end) return 0;
    bs = d->sb.blocksize;
    startblk = (u32)start / bs;
    endblk = (u32)end / bs;
    startoff = (u32)start % bs;
    endoff = (u32)end % bs;
    tmp = (u8 *)malloc(bs);
    if (!tmp) return 0;
    ofs = 0;
    for (lb = startblk; lb <= endblk; lb++)
    {
        u32 bofs = (lb == startblk) ? startoff : 0;
        u32 bend = (lb == endblk) ? endoff : bs - 1;
        u32 blen = bend - bofs + 1;
        u64 phys;
        int hole = ext4_map_block(c, lb, &phys);
        if (hole < 0)
        {
            memset(buf + ofs, 0, blen);
        }
        else
        {
            if (ext4_read_blocks(d, id, phys, 1, tmp) < 0)
            {
                free(tmp);
                return 0;
            }
            memcpy(buf + ofs, tmp + bofs, blen);
        }
        ofs += blen;
    }
    free(tmp);
    return (int)ofs;
}

static int ext4_writefile(vfs_node *f, char *buf, int start, int end, int id)
{
    ext4_dev *d = ext4_get_dev(id);
    ext4_ino *c;
    u32 bs, startblk, endblk, startoff, endoff, lb, ofs;
    u8 *tmp;
    if (!d) return 0;
    c = ext4_get_cache(f);
    if (!c) return 0;
    if (ext4_ensure(d, id, c) < 0) return 0;
    bs = d->sb.blocksize;
    startblk = (u32)start / bs;
    endblk = (u32)end / bs;
    startoff = (u32)start % bs;
    endoff = (u32)end % bs;
    tmp = (u8 *)malloc(bs);
    if (!tmp) return 0;
    ofs = 0;
    for (lb = startblk; lb <= endblk; lb++)
    {
        u32 bofs = (lb == startblk) ? startoff : 0;
        u32 bend = (lb == endblk) ? endoff : bs - 1;
        u32 blen = bend - bofs + 1;
        u64 phys;
        if (ext4_map_block(c, lb, &phys) < 0)
        {
            /* hole: zero-fill the affected region */
            memset(buf + ofs, 0, blen);
            ofs += blen;
            continue;
        }
        /* read-modify-write */
        if (bofs == 0 && blen == bs)
        {
            /* full block: DMA straight from buf */
            if (ext4_write_blocks(d, id, phys, 1, buf + ofs) < 0)
            {
                free(tmp);
                return 0;
            }
        }
        else
        {
            if (ext4_read_blocks(d, id, phys, 1, tmp) < 0)
            {
                free(tmp);
                return 0;
            }
            memcpy(tmp + bofs, buf + ofs, blen);
            if (ext4_write_blocks(d, id, phys, 1, tmp) < 0)
            {
                free(tmp);
                return 0;
            }
        }
        ofs += blen;
    }
    free(tmp);
    return (int)ofs;
}

static int ext4_addsectors(vfs_node *f, int nblocks, int id)
{
    ext4_dev *d = ext4_get_dev(id);
    ext4_ino *c;
    u64 fsize;
    u32 bs, cur_blocks, i, lastblock, runlen;
    u32 run_start, run_phys;
    if (!d) return -1;
    c = ext4_get_cache(f);
    if (!c) return -1;
    if (ext4_ensure(d, id, c) < 0) return -1;
    fsize = ((u64)c->size_hi << 32) | c->size_lo;
    bs = d->sb.blocksize;
    cur_blocks = (u32)((fsize + bs - 1) / bs);

    /* append nblocks new blocks as one or more runs */
    run_start = 0;
    run_phys = 0;
    runlen = 0;
    lastblock = (u32)-1;
    for (i = 0; i < (u32)nblocks; i++)
    {
        u32 b = ext4_alloc_block_raw(d, id);
        if (b == (u32)-1)
        {
            /* out of space */
            return -1;
        }
        if (runlen == 0)
        {
            run_start = cur_blocks + i;
            run_phys = b;
            lastblock = b;
            runlen = 1;
        }
        else if (b == lastblock + 1)
        {
            lastblock = b; /* keep tracking last */
            runlen++;
        }
        else
        {
            /* flush previous run (phys_lo = first allocated block) */
            if (c->nruns >= EXT4_MAX_RUNS) return -1;
            c->runs[c->nruns].log_block = run_start;
            c->runs[c->nruns].len = runlen;
            c->runs[c->nruns].phys_lo = run_phys;
            c->runs[c->nruns].phys_hi = 0;
            c->nruns++;
            run_start = cur_blocks + i;
            run_phys = b;
            lastblock = b;
            runlen = 1;
        }
    }
    if (runlen > 0)
    {
        if (c->nruns >= EXT4_MAX_RUNS) return -1;
        c->runs[c->nruns].log_block = run_start;
        c->runs[c->nruns].len = runlen;
        c->runs[c->nruns].phys_lo = run_phys;
        c->runs[c->nruns].phys_hi = 0;
        c->nruns++;
    }
    c->runs_loaded = 1;

    /* persist extent tree + inode */
    if (ext4_write_extents(d, id, c) < 0) return -1;
    c->nblocks = rd32(c->inode_raw + 0x1c) + (u32)nblocks * (bs / 512);
    wr32(c->inode_raw + 0x1c, c->nblocks);
    if (ext4_flush_ino(d, id, c) < 0) return -1;
    ext4_write_gdt(d, id);
    ext4_write_super(d, id);
    return nblocks;
}

static int ext4_createfile(vfs_node *f, int id)
{
    ext4_dev *d = ext4_get_dev(id);
    vfs_node *parent;
    ext4_ino *c, *pc;
    u8 *pimg;
    u32 pbytes;
    u32 ino;
    u8 raw[256];
    int is_dir;
    if (!d) return -1;
    parent = f->path;
    if (!parent) return -1;
    pimg = (u8 *)parent->misc2;
    if (!pimg) return -1;
    pbytes = parent->miscsize2;
    pc = ext4_get_cache(parent);
    if (!pc) return -1;

    is_dir = (f->attb & FILE_DIRECTORY) ? 1 : 0;

    ino = ext4_alloc_ino(d, id);
    if (ino == (u32)-1) return -1;

    /* initialize the inode */
    memset(raw, 0, d->sb.inode_size);
    if (is_dir)
        wr16(raw + 0x00, EXT4_S_IFDIR | 00755);
    else
        wr16(raw + 0x00, EXT4_S_IFREG | 00644);
    wr16(raw + 0x1a, (u16)(is_dir ? 2 : 1)); /* links */
    if (d->sb.inode_size > 128 && d->sb.want_extra_isize)
        wr16(raw + 0x80, (u16)d->sb.want_extra_isize);
    if (d->sb.has_extents)
        wr32(raw + 0x20, EXT4_EXTENTS_FL); /* inode uses extent tree */
    {
        dex32_datetime dt;
        getdatetime(&dt);
        wr32(raw + 0x08, (u32)dt.sec); /* atime (approx) */
        wr32(raw + 0x0c, (u32)dt.sec);
        wr32(raw + 0x10, (u32)dt.sec);
    }
    if (is_dir)
    {
        u32 blk = ext4_alloc_block_raw(d, id);
        ext4_run run;
        if (blk == (u32)-1) return -1;
        /* dir's first block: . and .. */
        {
            u8 dirblk[4096];
            ext4_dirent *de;
            u32 pino = pc->ino;
            memset(dirblk, 0, d->sb.blocksize);
            de = (ext4_dirent *)dirblk;
            de->inode = ino;
            de->rec_len = 12;
            de->name_len = 1;
            de->file_type = EXT4_FT_DIR;
            de->name[0] = '.';
            de = (ext4_dirent *)(dirblk + 12);
            de->inode = pino;
            /* leave 12 bytes for the csum tail entry when csums are on */
            de->rec_len = (u16)(d->sb.has_csum ? d->sb.blocksize - 24 : d->sb.blocksize - 12);
            de->name_len = 2;
            de->file_type = EXT4_FT_DIR;
            de->name[0] = '.'; de->name[1] = '.';
            if (d->sb.has_csum)
            {
                ext4_dirent *tail = (ext4_dirent *)(dirblk + d->sb.blocksize - 12);
                u8 seedbuf[8];
                u32 c2;
                tail->inode = 0;
                tail->rec_len = 12;
                tail->name_len = 0;
                tail->file_type = 0xde;
                le32buf(seedbuf, ino);
                le32buf(seedbuf + 4, 0);
                c2 = ext4_crc32c(seedbuf, d->sb.csum_seed, 8);
                c2 = ext4_crc32c(dirblk, c2, d->sb.blocksize - 12);
                wr32(dirblk + d->sb.blocksize - 4, c2);
            }
            if (ext4_write_blocks(d, id, blk, 1, dirblk) < 0) return -1;
        }
        run.log_block = 0;
        run.len = 1;
        run.phys_lo = blk;
        run.phys_hi = 0;
        /* set extent in raw i_block */
        {
            ext4_extent_header *hdr = (ext4_extent_header *)(raw + 0x28);
            ext4_extent *ext = (ext4_extent *)(raw + 0x28 + 12);
            hdr->eh_magic = EXT4_EXTENT_MAGIC;
            hdr->eh_entries = 1;
            hdr->eh_max = EXT4_INLINE_EXTENT_MAX;
            hdr->eh_depth = 0;
            hdr->eh_generation = 0;
            ext->ee_block = 0;
            ext->ee_len = 1;
            ext->ee_start_hi = 0;
            ext->ee_start_lo = blk;
        }
        wr32(raw + 0x1c, d->sb.blocksize / 512); /* i_blocks */
        wr32(raw + 0x04, d->sb.blocksize);       /* size = one block */
    }
    if (d->sb.has_csum)
        ext4_ino_set_csum(d, ino, 0, raw);
    if (ext4_write_ino(d, id, ino, 0, raw) < 0) return -1;

    /* add directory entry to parent image */
    {
       ext4_dirent *slot = ext4_alloc_entry(d, pimg, &pbytes, ino,
                                              f->name,
                                              (u8)(is_dir ? EXT4_FT_DIR : EXT4_FT_REG_FILE));
        if (!slot) return -1;
        /* write parent dir block back */
        ext4_write_parent_dir(d, id, parent, pimg, pbytes);
        c = (ext4_ino *)malloc(sizeof(ext4_ino));
        if (!c) return -1;
        memset(c, 0, sizeof(ext4_ino));
        c->ino = ino;
        c->is_dir = is_dir;
        c->dirent = (void *)slot;
        c->dirent_valid = 1;
        if (is_dir)
        {
            c->size_lo = d->sb.blocksize;
            c->nblocks = d->sb.blocksize / 512;
            c->mode = EXT4_S_IFDIR | 00755;
            f->attb = FILE_DIRECTORY | FILE_OREAD | FILE_OWRITE;
            f->files = VFS_NOT_MOUNTED;
        }
        else
        {
            c->mode = EXT4_S_IFREG | 00644;
            f->attb = FILE_OREAD | FILE_OWRITE | FILE_OEXE;
            f->files = 0;
        }
        f->misc = (void *)c;
        f->miscsize = sizeof(ext4_ino);
        f->size = is_dir ? d->sb.blocksize : 0;
        f->date_created.year = 2020;
    }
    if (is_dir)
    {
        u32 dg = (ino - 1) / d->sb.inodes_per_group;
        ext4_gd *dgd = &d->gdt[dg];
        u8 *ddesc = d->gdt_raw + dg * d->sb.desc_size;
        if (pc && pc->inode_loaded)
        {
            pc->links = (u16)(pc->links + 1);
            wr16(pc->inode_raw + 0x1a, pc->links);
            if (d->sb.has_csum)
                ext4_ino_set_csum(d, pc->ino, pc->generation, pc->inode_raw);
            ext4_write_ino(d, id, pc->ino, pc->generation, pc->inode_raw);
        }
        dgd->bg_used_dirs = (u16)(dgd->bg_used_dirs + 1);
        if (d->sb.desc_size >= 64) {
            wr16(ddesc + 0x10, (u16)(dgd->bg_used_dirs & 0xFFFF));
            wr16(ddesc + 0x30, (u16)((dgd->bg_used_dirs >> 16) & 0xFFFF));
        } else {
            wr16(ddesc + 0x10, dgd->bg_used_dirs);
        }
        ext4_gd_set_csum(d, dg);
    }
    ext4_write_gdt(d, id);
    ext4_write_super(d, id);
    return 0;
}

static int ext4_deletefile(vfs_node *f, int id)
{
    ext4_dev *d = ext4_get_dev(id);
    ext4_ino *c;
    if (!d) return -1;
    c = ext4_get_cache(f);
    if (!c) return -1;
    if (c->dirent_valid && c->dirent)
    {
        ext4_dirent *e = (ext4_dirent *)c->dirent;
        vfs_node *parent = f->path;
        u8 *pimg = (u8 *)parent->misc2;
        if (parent && pimg)
        {
            /* mark entry deleted: set inode to 0 and merge into previous,
               then persist the parent directory image */
            e->inode = 0;
            ext4_merge_delete(d, id, parent, pimg, e);
             ext4_write_parent_dir(d, id, parent, pimg, parent->miscsize2);
        }
    }
    /* free data blocks + inode (simplified: free runs) */
    if (ext4_ensure(d, id, c) == 0)
    {
        u32 i;
        for (i = 0; i < c->nruns; i++)
        {
            u32 j;
            for (j = 0; j < c->runs[i].len; j++)
                ext4_free_block_raw(d, id, c->runs[i].phys_lo + j);
        }
        c->nruns = 0;
        c->runs_loaded = 1;
    }
    ext4_free_ino_raw(d, id, c->ino);
    if (c->is_dir)
    {
        u32 dg = (c->ino - 1) / d->sb.inodes_per_group;
        ext4_gd *dgd = &d->gdt[dg];
        u8 *ddesc = d->gdt_raw + dg * d->sb.desc_size;
        if (dgd->bg_used_dirs > 0)
            dgd->bg_used_dirs = (u16)(dgd->bg_used_dirs - 1);
        if (d->sb.desc_size >= 64) {
            wr16(ddesc + 0x10, (u16)(dgd->bg_used_dirs & 0xFFFF));
            wr16(ddesc + 0x30, (u16)((dgd->bg_used_dirs >> 16) & 0xFFFF));
        } else {
            wr16(ddesc + 0x10, dgd->bg_used_dirs);
        }
        ext4_gd_set_csum(d, dg);
    }
    ext4_write_gdt(d, id);
    ext4_write_super(d, id);
    return 0;
}

static int ext4_chattb(vfs_node *f, const int attb, int id)
{
    ext4_dev *d = ext4_get_dev(id);
    ext4_ino *c;
    if (!d) return 0;
    c = ext4_get_cache(f);
    if (!c) return 0;
    if (attb & FILE_FSIZE)
    {
        if (ext4_ensure(d, id, c) < 0) return 0;
        c->size_lo = (u32)(f->size & 0xFFFFFFFF);
        c->size_hi = (u32)(f->size >> 32);
        wr32(c->inode_raw + 0x04, c->size_lo);
        wr32(c->inode_raw + 0x6c, c->size_hi);
        if (ext4_flush_ino(d, id, c) < 0) return 0;
    }
    return 0;
}

static int ext4_getbytesperblock(int id)
{
    ext4_dev *d = ext4_get_dev(id);
    if (!d) return 0;
    return (int)d->sb.blocksize;
}

static int ext4_getsectorsize(vfs_node *f, int id)
{
    ext4_dev *d = ext4_get_dev(id);
    ext4_ino *c;
    if (!d) return 0;
    c = ext4_get_cache(f);
    if (!c) return 0;
    if (ext4_ensure(d, id, c) < 0) return 0;
    return (int)(c->nblocks / (d->sb.blocksize / 512));
}

static int ext4_validate_filename(const char *filename)
{
    int len;
    if (!filename || !*filename) return 0;
    len = (int)strlen(filename);
    if (len > 255) return 0;
    if (len == 1 && filename[0] == '.') return 0;
    if (len == 2 && filename[0] == '.' && filename[1] == '.') return 0;
    return 1;
}

static int ext4_rewritefile(vfs_node *f, int id) { (void)f; (void)id; return 0; }
static int ext4_unmount(vfs_node *f)
{
    (void)f;
    return 0;
}

int ext4_register(const char *name)
{
    devmgr_fs_desc ext4_fs_desc;
    int ext4_deviceid;
    memset(&ext4_fs_desc, 0, sizeof(devmgr_fs_desc));
    strcpy(ext4_fs_desc.hdr.name, name);
    strcpy(ext4_fs_desc.hdr.description, "ext4 filesystem driver");
    ext4_fs_desc.hdr.type = DEVMGR_FS;
    ext4_fs_desc.hdr.size = sizeof(ext4_fs_desc);
    ext4_fs_desc.identify = ext4_identify;
    ext4_fs_desc.mountroot = ext4_mountroot;
    ext4_fs_desc.rewritefile = ext4_rewritefile;
    ext4_fs_desc.readfile = ext4_readfile;
    ext4_fs_desc.chattb = ext4_chattb;
    ext4_fs_desc.getsectorsize = ext4_getsectorsize;
    ext4_fs_desc.deletefile = ext4_deletefile;
    ext4_fs_desc.addsectors = ext4_addsectors;
    ext4_fs_desc.writefile = ext4_writefile;
    ext4_fs_desc.createfile = ext4_createfile;
    ext4_fs_desc.getbytesperblock = ext4_getbytesperblock;
    ext4_fs_desc.validate_filename = ext4_validate_filename;
    ext4_fs_desc.mountdirectory = ext4_mountdirectory;
    ext4_fs_desc.unmount = ext4_unmount;
    ext4_deviceid = devmgr_register((devmgr_generic *)&ext4_fs_desc);
    return ext4_deviceid;
}
