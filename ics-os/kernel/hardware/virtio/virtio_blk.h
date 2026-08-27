#ifndef ICSOS_VIRTIO_BLK_H
#define ICSOS_VIRTIO_BLK_H

#include "../../types.h"

void virtio_blk_init(void);
void virtio_blk_irq(void);

int  virtio_blk_present(void);
int  virtio_blk_readonly(void);
u64  virtio_blk_sectors(void);
void virtio_blk_harvest(void);

/* Queue one DMA request. done(arg, bytes_or_-errno) runs from harvest
 * (IRQ or poll). Returns 0 if queued. Sync callers pass done=0 and
 * wait with virtio_blk_wait(). */
int  virtio_blk_submit(u32 type, u64 sector, void *buf, u32 bytes,
                       void (*done)(void *arg, int res), void *arg,
                       int *slot_out);
int  virtio_blk_wait(int slot);

/* Byte-offset sync helpers for /dev/vblk. off and len must be 512-aligned. */
int  virtio_blk_rw(int write, u64 off, void *buf, u32 bytes);
int  virtio_blk_flush(void);

#endif
