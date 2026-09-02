/*
  Name: ata_capacity.h
  Description: Pure capacity-decision logic for ATA IDENTIFY data
  (ATA/ATAPI-8 Table 12 / QEMU ide_identify_size):

    words 60-61   28-bit LBA user capacity (bits 0-27; word 61 holds
                  bits 16-27 in its low 12 bits)
    words 100-103 48-bit LBA user capacity (bits 0-47)

  A drive must populate the 48-bit field once the 28-bit field would
  saturate, so the 48-bit value is authoritative when present.

  Self-contained (no includes) so the exact same code is compiled into
  the kernel (via ide.c) and unit-tested on the host.
*/
#ifndef ICSOS_ATA_CAPACITY_H
#define ICSOS_ATA_CAPACITY_H

/* Returns the user-addressable sector count reported by the device, or 0
   if no LBA capacity is reported (CHS-only legacy devices and ATAPI
   packet devices, which carry no sector count). */
static inline unsigned long long ata_capacity_sectors(const unsigned short *w)
{
    unsigned long long lba48;
    unsigned long long lba28;

    lba48 = (unsigned long long)w[100] |
            ((unsigned long long)w[101] << 16) |
            ((unsigned long long)w[102] << 32) |
            ((unsigned long long)w[103] << 48);
    lba28 = (unsigned long long)w[60] |
            (((unsigned long long)w[61] & 0x0FFFu) << 16);

    if (lba48)
        return lba48;
    if (lba28)
        return lba28;
    return 0;
}

#endif /* ICSOS_ATA_CAPACITY_H */
