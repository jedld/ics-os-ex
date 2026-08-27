/*
 * virtio-blk (OASIS virtio 1.2, modern PCI transport).
 *
 * One request queue, DMA via identity-mapped buffers, MSI-X on vector
 * 0x42. Each in-flight request owns a 3-descriptor chain. The used ring
 * is harvested in the MSI-X handler (and as a poll fallback); waiters
 * hlt until that IRQ instead of pause-spinning. ATA PIO remains the
 * fallback when QEMU has no virtio disk.
 */
#include "virtio.h"
#include "virtio_blk.h"
#include "../../devmgr/dex32_devmgr.h"
#include "../../cpu/lapic.h"
#include "../../process/process.h"
#include "../../stdlib/time.h"

extern void *malloc(unsigned int);
extern void *memset(void *s, int c, unsigned int n);
extern int memcmp(const void *s1, const void *s2, unsigned int n);
extern char *strcpy(char *d, const char *s);
extern int printf(const char *fmt, ...);
extern void *memcpy(void *d, const void *s, unsigned int n);
extern void mmio_mark_uncacheable(u64 phys, u64 len);
extern void storeflags(DWORD *flags);
extern void restoreflags(DWORD flags);
extern void stopints(void);
extern unsigned int ticks;

#define VBLK_DESCS_PER_REQ 3
#define VBLK_BOUNCE        4096
#define VBLK_TIMEOUT_TICKS 500
#define VBLK_EIO           (-5)
#define VBLK_EINVAL        (-22)
#define VBLK_ENOMEM        (-12)

#define SYS_CODE_SEL 0x08
typedef struct __attribute__((packed)) _idtentry_v {
   WORD lowphy;
   WORD selector;
   BYTE ist;
   BYTE attr;
   WORD midphy;
   DWORD highphy;
   DWORD reserved;
} idtentry_v;

extern idtentry_v *dex_idtbase;
extern void setinterruptvector(DWORD index, idtentry_v *t, unsigned char attr,
                               void (*handler)(int irq), WORD sel);
extern void virtio_msixwrapper(void);

#define PCI_CONFIG_ADDR  0xCF8
#define PCI_CONFIG_DATA  0xCFC
#define PCI_CMD          0x04
#define PCI_STATUS       0x06
#define PCI_BAR0         0x10
#define PCI_CAP_PTR      0x34
#define PCI_CMD_MEMORY   0x0002
#define PCI_CMD_MASTER   0x0004
#define PCI_CMD_INTX_OFF 0x0400
#define PCI_STATUS_CAPS  0x0010

static volatile int vblk_irq_count;

static inline void virt_mb(void)
{
   __asm__ volatile ("mfence" ::: "memory");
}

static inline u32 pci_inl(u16 port)
{
   u32 v;
   __asm__ volatile ("inl %%dx, %%eax" : "=a"(v) : "d"(port));
   return v;
}

static inline void pci_outl(u16 port, u32 val)
{
   __asm__ volatile ("outl %%eax, %%dx" : : "d"(port), "a"(val));
}

static inline u32 pci_addr(u8 bus, u8 dev, u8 fn, u8 off)
{
   return 0x80000000u | ((u32)bus << 16) | ((u32)dev << 11) |
          ((u32)fn << 8) | (off & 0xFC);
}

static u32 pci_read32(u8 bus, u8 dev, u8 fn, u8 off)
{
   pci_outl(PCI_CONFIG_ADDR, pci_addr(bus, dev, fn, off));
   return pci_inl(PCI_CONFIG_DATA);
}

static void pci_write32(u8 bus, u8 dev, u8 fn, u8 off, u32 val)
{
   pci_outl(PCI_CONFIG_ADDR, pci_addr(bus, dev, fn, off));
   pci_outl(PCI_CONFIG_DATA, val);
}

static u16 pci_read16(u8 bus, u8 dev, u8 fn, u8 off)
{
   u32 v = pci_read32(bus, dev, fn, off & 0xFC);
   return (u16)(v >> ((off & 2) * 8));
}

static u8 pci_read8(u8 bus, u8 dev, u8 fn, u8 off)
{
   u32 v = pci_read32(bus, dev, fn, off & 0xFC);
   return (u8)(v >> ((off & 3) * 8));
}

static void pci_write16(u8 bus, u8 dev, u8 fn, u8 off, u16 val)
{
   u32 v = pci_read32(bus, dev, fn, off & 0xFC);
   u32 sh = (off & 2) * 8;
   v = (v & ~(0xFFFFu << sh)) | ((u32)val << sh);
   pci_write32(bus, dev, fn, off & 0xFC, v);
}

static u64 pci_read_bar(u8 bus, u8 dev, u8 fn, u8 bar)
{
   u32 lo, hi;
   u8 off = (u8)(PCI_BAR0 + bar * 4);
   lo = pci_read32(bus, dev, fn, off);
   if (lo & 1)
      return (u64)(lo & ~3u);
   if ((lo & 6) == 4) {
      hi = pci_read32(bus, dev, fn, (u8)(off + 4));
      return ((u64)hi << 32) | (lo & ~0xFu);
   }
   return (u64)(lo & ~0xFu);
}

static inline u8 mmio_r8(volatile u8 *p) { return *p; }
static inline u16 mmio_r16(volatile u16 *p) { return *p; }
static inline u32 mmio_r32(volatile u32 *p) { return *p; }
static inline u64 mmio_r64(volatile u64 *p) { return *p; }

static inline void mmio_w8(volatile u8 *p, u8 v)
{
   *p = v;
   virt_mb();
}
static inline void mmio_w16(volatile u16 *p, u16 v)
{
   *p = v;
   virt_mb();
}
static inline void mmio_w32(volatile u32 *p, u32 v)
{
   *p = v;
   virt_mb();
}
static inline void mmio_w64(volatile u64 *p, u64 v)
{
   *p = v;
   virt_mb();
}

struct vblk_slot {
   struct virtio_blk_req req;
   u32 nbytes;
   int res;
   PCB386 *waiter;
   void (*done_fn)(void *arg, int res);
   void *done_arg;
   void *user_buf;
   volatile u8 done;
   u8 busy;
   u8 status;
   u8 copy_back;
   u8 pad[4];
};
typedef char vblk_slot_sz[(sizeof(struct vblk_slot) == 64) ? 1 : -1];

struct vblk_dev {
   u8 bus, dev, fn;
   volatile struct virtio_pci_common_cfg *common;
   volatile u8 *notify;
   volatile u8 *isr;
   volatile u8 *devcfg;
   u32 notify_off_mul;
   u16 qsize;
   u16 nslots;
   u16 notify_off;
   volatile struct virtq_desc *desc;
   volatile struct virtq_avail *avail;
   volatile struct virtq_used *used;
   struct vblk_slot *slots;
   u8 *bounce;
   u16 avail_idx;
   u16 last_used;
   u64 capacity;
   u32 blk_size;
   u64 features;
   int readonly;
   int has_flush;
   int has_msix;
   int deviceid;
};

static struct vblk_dev vblk;
static int vblk_ready;

static void *zalloc_align(unsigned sz, unsigned align)
{
   uintptr p;
   unsigned extra = align + 32;
   p = (uintptr)malloc(sz + extra);
   if (!p)
      return 0;
   p = (p + (align - 1)) & ~(uintptr)(align - 1);
   memset((void *)p, 0, sz);
   return (void *)p;
}

static int virtio_find_blk(u8 *bus, u8 *dev, u8 *fn)
{
   int b, d, f;
   for (b = 0; b < 8; b++) {
      for (d = 0; d < 32; d++) {
         for (f = 0; f < 8; f++) {
            u16 vend = pci_read16((u8)b, (u8)d, (u8)f, 0);
            u16 did;
            if (vend != VIRTIO_VENDOR_ID)
               continue;
            did = pci_read16((u8)b, (u8)d, (u8)f, 2);
            if (did == VIRTIO_DEV_BLK_TRANS || did == VIRTIO_DEV_BLK_MODERN) {
               *bus = (u8)b;
               *dev = (u8)d;
               *fn = (u8)f;
               return 1;
            }
         }
      }
   }
   return 0;
}

static volatile u8 *map_cap(u8 bus, u8 dev, u8 fn, u8 bar, u32 offset, u32 length)
{
   u64 base;
   if (bar > 5 || length == 0)
      return 0;
   base = pci_read_bar(bus, dev, fn, bar);
   if (base < 0x100000ULL)
      return 0;
   mmio_mark_uncacheable(base + offset, length ? length : 0x1000);
   return (volatile u8 *)(uintptr)(base + offset);
}

static int virtio_parse_caps(struct vblk_dev *d)
{
   u16 status = pci_read16(d->bus, d->dev, d->fn, PCI_STATUS);
   u8 ptr;
   if (!(status & PCI_STATUS_CAPS))
      return 0;
   ptr = pci_read8(d->bus, d->dev, d->fn, PCI_CAP_PTR);
   while (ptr >= 0x40) {
      u8 id = pci_read8(d->bus, d->dev, d->fn, ptr);
      u8 next = pci_read8(d->bus, d->dev, d->fn, (u8)(ptr + 1));
      if (id == VIRTIO_PCI_CAP_VNDR) {
         u8 cfg_type = pci_read8(d->bus, d->dev, d->fn, (u8)(ptr + 3));
         u8 bar = pci_read8(d->bus, d->dev, d->fn, (u8)(ptr + 4));
         u32 offset = pci_read32(d->bus, d->dev, d->fn, (u8)(ptr + 8));
         u32 length = pci_read32(d->bus, d->dev, d->fn, (u8)(ptr + 12));
         volatile u8 *p = map_cap(d->bus, d->dev, d->fn, bar, offset, length);
         if (cfg_type == VIRTIO_PCI_CAP_COMMON_CFG)
            d->common = (volatile struct virtio_pci_common_cfg *)p;
         else if (cfg_type == VIRTIO_PCI_CAP_NOTIFY_CFG) {
            d->notify = p;
            d->notify_off_mul = pci_read32(d->bus, d->dev, d->fn, (u8)(ptr + 16));
         } else if (cfg_type == VIRTIO_PCI_CAP_ISR_CFG)
            d->isr = p;
         else if (cfg_type == VIRTIO_PCI_CAP_DEVICE_CFG)
            d->devcfg = p;
      }
      ptr = next;
      if (ptr == 0)
         break;
   }
   return d->common != 0 && d->notify != 0 && d->devcfg != 0;
}

static int virtio_setup_msix(struct vblk_dev *d)
{
   u16 status = pci_read16(d->bus, d->dev, d->fn, PCI_STATUS);
   u8 ptr;
   u32 table_bir, pba_bir;
   u16 ctrl;
   u64 table_base;
   volatile u32 *entry;
   u32 apic_id;
   u16 cmd;

   if (!(status & PCI_STATUS_CAPS))
      return 0;
   ptr = pci_read8(d->bus, d->dev, d->fn, PCI_CAP_PTR);
   while (ptr >= 0x40) {
      u8 id = pci_read8(d->bus, d->dev, d->fn, ptr);
      u8 next = pci_read8(d->bus, d->dev, d->fn, (u8)(ptr + 1));
      if (id == PCI_CAP_ID_MSIX) {
         ctrl = pci_read16(d->bus, d->dev, d->fn, (u8)(ptr + 2));
         table_bir = pci_read32(d->bus, d->dev, d->fn, (u8)(ptr + 4));
         pba_bir = pci_read32(d->bus, d->dev, d->fn, (u8)(ptr + 8));
         (void)pba_bir;
         table_base = pci_read_bar(d->bus, d->dev, d->fn, (u8)(table_bir & 7));
         table_base += (table_bir & ~7u);
         mmio_mark_uncacheable(table_base, 0x1000);
         entry = (volatile u32 *)(uintptr)table_base;
         apic_id = lapic_get_id();
         entry[0] = 0xFEE00000u | (apic_id << 12);
         entry[1] = 0;
         entry[2] = VIRTIO_MSIX_VECTOR;
         entry[3] = 0;
         virt_mb();
         setinterruptvector(VIRTIO_MSIX_VECTOR, dex_idtbase, 0x8E,
                            (void (*)(int))virtio_msixwrapper, SYS_CODE_SEL);
         cmd = pci_read16(d->bus, d->dev, d->fn, PCI_CMD);
         pci_write16(d->bus, d->dev, d->fn, PCI_CMD,
                     (u16)(cmd | PCI_CMD_INTX_OFF));
         pci_write16(d->bus, d->dev, d->fn, (u8)(ptr + 2),
                     (u16)(ctrl | 0x8000));
         d->has_msix = 1;
         return 1;
      }
      ptr = next;
      if (ptr == 0)
         break;
   }
   return 0;
}

static u64 virtio_read_features(struct vblk_dev *d)
{
   u32 lo, hi;
   mmio_w32(&d->common->device_feature_select, 0);
   lo = mmio_r32(&d->common->device_feature);
   mmio_w32(&d->common->device_feature_select, 1);
   hi = mmio_r32(&d->common->device_feature);
   return ((u64)hi << 32) | lo;
}

static void virtio_write_features(struct vblk_dev *d, u64 f)
{
   mmio_w32(&d->common->driver_feature_select, 0);
   mmio_w32(&d->common->driver_feature, (u32)f);
   mmio_w32(&d->common->driver_feature_select, 1);
   mmio_w32(&d->common->driver_feature, (u32)(f >> 32));
}

static int virtio_setup_queue(struct vblk_dev *d)
{
   u16 qsz;
   mmio_w16(&d->common->queue_select, 0);
   qsz = mmio_r16(&d->common->queue_size);
   if (qsz == 0)
      return 0;
   if (qsz > VIRTIO_QUEUE_MAX) {
      qsz = VIRTIO_QUEUE_MAX;
      mmio_w16(&d->common->queue_size, qsz);
   }
   d->qsize = qsz;
   d->nslots = (u16)(qsz / VBLK_DESCS_PER_REQ);
   if (d->nslots < 1)
      return 0;
   d->desc = zalloc_align(sizeof(struct virtq_desc) * qsz, 16);
   d->avail = zalloc_align(sizeof(struct virtq_avail), 2);
   d->used = zalloc_align(sizeof(struct virtq_used), 4);
   d->slots = zalloc_align(sizeof(struct vblk_slot) * d->nslots, 64);
   d->bounce = zalloc_align((unsigned)d->nslots * VBLK_BOUNCE, 4096);
   if (!d->desc || !d->avail || !d->used || !d->slots || !d->bounce)
      return 0;

   mmio_w16(&d->common->queue_msix_vector,
            d->has_msix ? 0 : VIRTIO_MSI_NO_VECTOR);
   mmio_w16(&d->common->msix_config, VIRTIO_MSI_NO_VECTOR);
   mmio_w64(&d->common->queue_desc, (u64)(uintptr)d->desc);
   mmio_w64(&d->common->queue_driver, (u64)(uintptr)d->avail);
   mmio_w64(&d->common->queue_device, (u64)(uintptr)d->used);
   d->notify_off = mmio_r16(&d->common->queue_notify_off);
   mmio_w16(&d->common->queue_enable, 1);
   return 1;
}

static void virtio_notify(struct vblk_dev *d)
{
   volatile u16 *p;
   u32 off = (u32)d->notify_off * d->notify_off_mul;
   p = (volatile u16 *)(d->notify + off);
   mmio_w16(p, 0);
}

static void vblk_complete_slot(struct vblk_slot *s)
{
   int res;

   if (s->status == VIRTIO_BLK_S_OK)
      res = (int)s->nbytes;
   else
      res = VBLK_EIO;
   s->res = res;
   s->done = 1;
   if (s->waiter)
      s->waiter->waiting = 0;
}

static void vblk_finish_async(struct vblk_dev *d, int si)
{
   struct vblk_slot *s = &d->slots[si];
   void (*fn)(void *arg, int res);
   void *arg;

   if (!s->done || !s->busy || !s->done_fn)
      return;
   if (s->copy_back && s->user_buf && s->res > 0)
      memcpy(s->user_buf, d->bounce + (unsigned)si * VBLK_BOUNCE,
             (unsigned)s->res);
   fn = s->done_fn;
   arg = s->done_arg;
   s->done_fn = 0;
   s->busy = 0;
   fn(arg, s->res);
}

static void vblk_harvest_locked(struct vblk_dev *d)
{
   u16 used_idx;

   if (!d->used || !d->slots)
      return;
   virt_mb();
   used_idx = d->used->idx;
   while (d->last_used != used_idx) {
      struct virtq_used_elem *e;
      u16 head, si;
      struct vblk_slot *s;

      e = &d->used->ring[d->last_used % d->qsize];
      head = (u16)e->id;
      si = (u16)(head / VBLK_DESCS_PER_REQ);
      d->last_used++;
      if (si >= d->nslots)
         continue;
      s = &d->slots[si];
      if (!s->busy)
         continue;
      vblk_complete_slot(s);
   }
}

void virtio_blk_harvest(void)
{
   DWORD flags;
   int i;

   if (!vblk_ready)
      return;
   storeflags(&flags);
   stopints();
   vblk_harvest_locked(&vblk);
   restoreflags(flags);
   for (i = 0; i < (int)vblk.nslots; i++)
      vblk_finish_async(&vblk, i);
}

static int vblk_alloc_slot(struct vblk_dev *d)
{
   u16 i;
   for (i = 0; i < d->nslots; i++) {
      if (!d->slots[i].busy) {
         d->slots[i].busy = 1;
         d->slots[i].done = 0;
         d->slots[i].res = 0;
         d->slots[i].waiter = 0;
         d->slots[i].done_fn = 0;
         d->slots[i].done_arg = 0;
         d->slots[i].user_buf = 0;
         d->slots[i].copy_back = 0;
         return (int)i;
      }
   }
   return -1;
}

int virtio_blk_submit(u32 type, u64 sector, void *buf, u32 bytes,
                      void (*done)(void *arg, int res), void *arg,
                      int *slot_out)
{
   struct vblk_dev *d = &vblk;
   struct vblk_slot *s;
   DWORD flags;
   unsigned start;
   int si, has_data;
   u16 head, i;

   if (!vblk_ready)
      return VBLK_EIO;
   if (type == VIRTIO_BLK_T_FLUSH && !d->has_flush) {
      if (done)
         done(arg, 0);
      if (slot_out)
         *slot_out = -1;
      return 0;
   }
   if (type != VIRTIO_BLK_T_FLUSH) {
      if (!buf || bytes == 0 || bytes > VBLK_BOUNCE ||
          (bytes & (VIRTIO_BLK_SECTOR_SIZE - 1)))
         return VBLK_EINVAL;
      if (sector + bytes / VIRTIO_BLK_SECTOR_SIZE > d->capacity)
         return VBLK_EINVAL;
   }
   if (type == VIRTIO_BLK_T_OUT && d->readonly)
      return VBLK_EIO;

   start = ticks;
   for (;;) {
      storeflags(&flags);
      stopints();
      vblk_harvest_locked(d);
      si = vblk_alloc_slot(d);
      if (si >= 0)
         break;
      restoreflags(flags);
      for (i = 0; i < (int)d->nslots; i++)
         vblk_finish_async(d, i);
      if (ticks - start > VBLK_TIMEOUT_TICKS)
         return VBLK_ENOMEM;
      cpu_idle();
   }

   s = &d->slots[si];
   s->nbytes = (type == VIRTIO_BLK_T_FLUSH) ? 0 : bytes;
   s->done_fn = done;
   s->done_arg = arg;
   s->user_buf = buf;
   s->copy_back = (type == VIRTIO_BLK_T_IN) ? 1 : 0;
   if (!done && current_process)
      s->waiter = current_process;
   s->req.type = type;
   s->req.reserved = 0;
   s->req.sector = sector;
   s->status = 0xFF;

   head = (u16)(si * VBLK_DESCS_PER_REQ);
   has_data = (type != VIRTIO_BLK_T_FLUSH) && bytes != 0;

   d->desc[head].addr = (u64)(uintptr)&s->req;
   d->desc[head].len = sizeof(struct virtio_blk_req);
   d->desc[head].flags = VIRTQ_DESC_F_NEXT;
   d->desc[head].next = has_data ? (u16)(head + 1) : (u16)(head + 2);

   if (has_data) {
      u8 *bnc = d->bounce + (unsigned)si * VBLK_BOUNCE;
      if (type == VIRTIO_BLK_T_OUT)
         memcpy(bnc, buf, bytes);
      d->desc[head + 1].addr = (u64)(uintptr)bnc;
      d->desc[head + 1].len = bytes;
      d->desc[head + 1].flags = VIRTQ_DESC_F_NEXT;
      if (type == VIRTIO_BLK_T_IN)
         d->desc[head + 1].flags |= VIRTQ_DESC_F_WRITE;
      d->desc[head + 1].next = (u16)(head + 2);
   }

   i = (u16)(head + 2);
   d->desc[i].addr = (u64)(uintptr)&s->status;
   d->desc[i].len = 1;
   d->desc[i].flags = VIRTQ_DESC_F_WRITE;
   d->desc[i].next = 0;

   d->avail->flags = 0;
   d->avail->ring[d->avail_idx % d->qsize] = head;
   virt_mb();
   d->avail_idx++;
   d->avail->idx = d->avail_idx;
   virt_mb();
   virtio_notify(d);
   restoreflags(flags);

   if (slot_out)
      *slot_out = si;
   return 0;
}

int virtio_blk_wait(int slot)
{
   struct vblk_dev *d = &vblk;
   struct vblk_slot *s;
   unsigned start;
   int res;

   if (!vblk_ready || slot < 0 || slot >= (int)d->nslots)
      return VBLK_EINVAL;
   s = &d->slots[slot];
   start = ticks;
   while (!s->done) {
      virtio_blk_harvest();
      if (s->done)
         break;
      if (ticks - start > VBLK_TIMEOUT_TICKS) {
         printf("virtio-blk: timeout slot=%d\n", slot);
         s->busy = 0;
         return VBLK_EIO;
      }
      cpu_idle();
   }
   res = s->res;
   if (s->copy_back && s->user_buf && res > 0)
      memcpy(s->user_buf, d->bounce + (unsigned)slot * VBLK_BOUNCE,
             (unsigned)res);
   s->busy = 0;
   return res;
}

static int vblk_xfer(struct vblk_dev *d, u32 type, u64 sector,
                     void *buf, u32 bytes)
{
   int slot, n;

   (void)d;
   if (virtio_blk_submit(type, sector, buf, bytes, 0, 0, &slot) < 0)
      return 0;
   n = virtio_blk_wait(slot);
   if (type == VIRTIO_BLK_T_FLUSH)
      return n >= 0;
   return n == (int)bytes;
}

static int vblk_rw_chunks(u32 type, u64 block, char *buf, u64 n)
{
   while (n) {
      u32 chunk = (n > (u64)(VBLK_BOUNCE / VIRTIO_BLK_SECTOR_SIZE)) ?
                  (u32)(VBLK_BOUNCE / VIRTIO_BLK_SECTOR_SIZE) : (u32)n;
      if (!vblk_xfer(&vblk, type, block, buf,
                     chunk * VIRTIO_BLK_SECTOR_SIZE))
         return 0;
      buf += chunk * VIRTIO_BLK_SECTOR_SIZE;
      block += chunk;
      n -= chunk;
   }
   return 1;
}

static int vblk_read_block(u64 block, char *buf, DWORD numblocks)
{
   u64 n = numblocks;
   if (!vblk_ready || !buf || n == 0)
      return 0;
   if (block + n > vblk.capacity)
      return 0;
   return vblk_rw_chunks(VIRTIO_BLK_T_IN, block, buf, n);
}

static int vblk_write_block(u64 block, char *buf, DWORD numblocks)
{
   u64 n = numblocks;
   if (!vblk_ready || vblk.readonly || !buf || n == 0)
      return 0;
   if (block + n > vblk.capacity)
      return 0;
   return vblk_rw_chunks(VIRTIO_BLK_T_OUT, block, buf, n);
}

static int vblk_total_blocks(void)
{
   if (vblk.capacity > 0x7FFFFFFFULL)
      return 0x7FFFFFFF;
   return (int)vblk.capacity;
}

static int vblk_get_block_size(void)
{
   return VIRTIO_BLK_SECTOR_SIZE;
}

static int vblk_flush_device(void)
{
   if (!vblk_ready)
      return 1;
   if (!vblk.has_flush)
      return 1;
   return vblk_xfer(&vblk, VIRTIO_BLK_T_FLUSH, 0, 0, 0);
}

static void vblk_selftest(void)
{
   char *a, *b;
   unsigned i;
   u64 sector;

   if (vblk.capacity < 32) {
      printf("virtio-blk: capacity too small\n");
      printf("VIRTIO_BLK_FAIL\n");
      return;
   }
   a = (char *)malloc(VIRTIO_BLK_SECTOR_SIZE);
   b = (char *)malloc(VIRTIO_BLK_SECTOR_SIZE);
   if (!a || !b) {
      printf("VIRTIO_BLK_FAIL\n");
      return;
   }
   sector = vblk.capacity - 1;
   for (i = 0; i < VIRTIO_BLK_SECTOR_SIZE; i++)
      a[i] = (char)(0xA5 ^ (i & 0xFF));
   if (!vblk_write_block(sector, a, 1) ||
       !vblk_read_block(sector, b, 1) ||
       memcmp(a, b, VIRTIO_BLK_SECTOR_SIZE) != 0) {
      printf("virtio-blk: R/W mismatch at LBA %llu\n",
             (unsigned long long)sector);
      printf("VIRTIO_BLK_FAIL\n");
      return;
   }
   printf("virtio-blk: dma R/W LBA %llu [OK]\n", (unsigned long long)sector);

   {
      int s0, s1;
      char *p0, *p1;
      p0 = (char *)malloc(VIRTIO_BLK_SECTOR_SIZE);
      p1 = (char *)malloc(VIRTIO_BLK_SECTOR_SIZE);
      if (!p0 || !p1 ||
          virtio_blk_submit(VIRTIO_BLK_T_IN, sector, p0,
                            VIRTIO_BLK_SECTOR_SIZE, 0, 0, &s0) < 0 ||
          virtio_blk_submit(VIRTIO_BLK_T_IN, sector, p1,
                            VIRTIO_BLK_SECTOR_SIZE, 0, 0, &s1) < 0 ||
          virtio_blk_wait(s0) != (int)VIRTIO_BLK_SECTOR_SIZE ||
          virtio_blk_wait(s1) != (int)VIRTIO_BLK_SECTOR_SIZE ||
          memcmp(p0, a, VIRTIO_BLK_SECTOR_SIZE) != 0 ||
          memcmp(p1, a, VIRTIO_BLK_SECTOR_SIZE) != 0) {
         printf("virtio-blk: pipelined reads [FAIL]\n");
         printf("VIRTIO_BLK_FAIL\n");
         return;
      }
      printf("virtio-blk: pipelined reads [OK]\n");
   }

   printf("virtio-blk: irqs=%d slots=%u\n",
          vblk_irq_count, (unsigned)vblk.nslots);
   if (vblk_irq_count > 0)
      printf("VIRTIO_IRQ_OK\n");
   printf("VIRTIO_BLK_OK\n");
}

int virtio_blk_present(void)
{
   return vblk_ready;
}

int virtio_blk_readonly(void)
{
   return vblk.readonly;
}

u64 virtio_blk_sectors(void)
{
   return vblk.capacity;
}

int virtio_blk_rw(int write, u64 off, void *buf, u32 bytes)
{
   u64 sector;
   int slot, n;

   if (!vblk_ready)
      return VBLK_EIO;
   if (!buf || bytes == 0)
      return 0;
   if ((off | (u64)bytes) & (VIRTIO_BLK_SECTOR_SIZE - 1))
      return VBLK_EINVAL;
   sector = off / VIRTIO_BLK_SECTOR_SIZE;
   n = virtio_blk_submit(write ? VIRTIO_BLK_T_OUT : VIRTIO_BLK_T_IN,
                         sector, buf, bytes, 0, 0, &slot);
   if (n < 0)
      return n;
   return virtio_blk_wait(slot);
}

int virtio_blk_flush(void)
{
   int slot, n;

   if (!vblk_ready)
      return 0;
   if (!vblk.has_flush)
      return 0;
   n = virtio_blk_submit(VIRTIO_BLK_T_FLUSH, 0, 0, 0, 0, 0, &slot);
   if (n < 0)
      return n;
   n = virtio_blk_wait(slot);
   return n < 0 ? n : 0;
}

void virtio_blk_irq(void)
{
   vblk_irq_count++;
   if (vblk.isr)
      (void)*vblk.isr;
   if (vblk_ready)
      vblk_harvest_locked(&vblk);
   lapic_eoi();
}

void virtio_blk_init(void)
{
   devmgr_block_desc myblock;
   u64 offered, wanted;
   u8 st;

   memset(&vblk, 0, sizeof(vblk));
   vblk.deviceid = -1;

   if (!virtio_find_blk(&vblk.bus, &vblk.dev, &vblk.fn)) {
      printf("virtio-blk: none\n");
      return;
   }

   printf("virtio-blk: pci %d:%d.%d\n", vblk.bus, vblk.dev, vblk.fn);
   pci_write16(vblk.bus, vblk.dev, vblk.fn, PCI_CMD,
               PCI_CMD_MEMORY | PCI_CMD_MASTER);

   if (!virtio_parse_caps(&vblk)) {
      printf("virtio-blk: no modern virtio-pci caps\n");
      return;
   }

   mmio_w8(&vblk.common->device_status, 0);
   mmio_w8(&vblk.common->device_status, VIRTIO_STATUS_ACKNOWLEDGE);
   mmio_w8(&vblk.common->device_status,
           VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER);

   offered = virtio_read_features(&vblk);
   wanted = (1ULL << VIRTIO_F_VERSION_1);
   if (offered & (1ULL << VIRTIO_BLK_F_FLUSH))
      wanted |= (1ULL << VIRTIO_BLK_F_FLUSH);
   if (offered & (1ULL << VIRTIO_BLK_F_BLK_SIZE))
      wanted |= (1ULL << VIRTIO_BLK_F_BLK_SIZE);
   if (offered & (1ULL << VIRTIO_BLK_F_RO))
      wanted |= (1ULL << VIRTIO_BLK_F_RO);
   vblk.features = wanted;
   vblk.readonly = (wanted & (1ULL << VIRTIO_BLK_F_RO)) != 0;
   vblk.has_flush = (wanted & (1ULL << VIRTIO_BLK_F_FLUSH)) != 0;
   virtio_write_features(&vblk, wanted);

   st = (u8)(VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER |
             VIRTIO_STATUS_FEATURES_OK);
   mmio_w8(&vblk.common->device_status, st);
   if (!(mmio_r8(&vblk.common->device_status) & VIRTIO_STATUS_FEATURES_OK)) {
      printf("virtio-blk: FEATURES_OK rejected\n");
      mmio_w8(&vblk.common->device_status, VIRTIO_STATUS_FAILED);
      return;
   }

   virtio_setup_msix(&vblk);
   if (!virtio_setup_queue(&vblk)) {
      printf("virtio-blk: queue setup failed\n");
      mmio_w8(&vblk.common->device_status, VIRTIO_STATUS_FAILED);
      return;
   }

   vblk.capacity = mmio_r64((volatile u64 *)vblk.devcfg);
   vblk.blk_size = VIRTIO_BLK_SECTOR_SIZE;
   if (vblk.features & (1ULL << VIRTIO_BLK_F_BLK_SIZE))
      vblk.blk_size = mmio_r32((volatile u32 *)(vblk.devcfg + 20));
   if (vblk.blk_size != VIRTIO_BLK_SECTOR_SIZE)
      printf("virtio-blk: logical block %u (using 512B BIOS LBAs)\n",
             vblk.blk_size);

   mmio_w8(&vblk.common->device_status,
           (u8)(st | VIRTIO_STATUS_DRIVER_OK));
   vblk_ready = 1;

   memset(&myblock, 0, sizeof(myblock));
   strcpy(myblock.hdr.name, "vblk");
   strcpy(myblock.hdr.description, "virtio-blk 1.2 (DMA)");
   myblock.hdr.type = DEVMGR_BLOCK;
   myblock.hdr.size = sizeof(myblock);
   myblock.read_block = vblk_read_block;
   myblock.write_block = vblk.readonly ? 0 : vblk_write_block;
   myblock.get_block_size = vblk_get_block_size;
   myblock.total_blocks = vblk_total_blocks;
   myblock.flush_device = vblk_flush_device;
   vblk.deviceid = devmgr_register((devmgr_generic *)&myblock);

   printf("virtio-blk: capacity=%llu sectors msix=%d devid=%d\n",
          (unsigned long long)vblk.capacity, vblk.has_msix, vblk.deviceid);
   vblk_selftest();
}
