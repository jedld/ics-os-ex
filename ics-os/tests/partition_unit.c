/*
  Name: partition_unit.c
  Description: Host unit tests for the GPT Phase-0 partition primitives:
  standard CRC-32 (kernel/partition/crc32.c) and the ATA capacity
  decision logic (kernel/hardware/ATA/ata_capacity.h). Both sources are
  self-contained and compile unmodified on the host.

  CRC-32 reference vectors are the zlib/IEEE 802.3 values:
    empty            -> 0x00000000
    "123456789"      -> 0xCBF43926
    0x00..0xFF       -> 0x29058C73
    512B (i*7+13)    -> 0x367ED872
  Chunked processing must equal one-shot processing (GPT headers are
  checked per 512-byte block, but the parser may feed any chunk size).
*/
#include <stdio.h>
#include <string.h>

#include "kernel/partition/crc32.h"
#include "kernel/partition/crc32.c"
#include "kernel/hardware/ATA/ata_capacity.h"

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
    static unsigned char pattern[512];
    static unsigned short id[104];
    unsigned int c;
    unsigned int i;

    printf("TAP version 13\n1..15\n");

    for (i = 0; i < sizeof(pattern); i++)
        pattern[i] = (unsigned char)((i * 7 + 13) & 0xFF);

    /* --- CRC-32 ---------------------------------------------------- */
    ok &= check("crc32 of an empty buffer is zero",
                crc32_ieee(pattern, 0, 0) == 0x00000000u);
    ok &= check("crc32 of \"123456789\" matches the IEEE check value",
                crc32_ieee("123456789", 0, 9) == 0xCBF43926u);
    {
        static unsigned char ramp[256];
        unsigned int j;
        for (j = 0; j < 256; j++)
            ramp[j] = (unsigned char)j;
        ok &= check("crc32 of 0x00..0xFF matches the reference vector",
                    crc32_ieee(ramp, 0, 256) == 0x29058C73u);
    }
    ok &= check("crc32 of the 512-byte pattern matches the reference vector",
                crc32_ieee(pattern, 0, 512) == 0x367ED872u);

    /* chunked == one-shot at unaligned boundaries */
    c = crc32_ieee(pattern, 0, 7);
    ok &= check("crc32 chunked at byte 7 equals one-shot",
                crc32_ieee(pattern + 7, c, 505) == 0x367ED872u);
    c = crc32_ieee(pattern, 0, 1);
    ok &= check("crc32 chunked byte-by-byte prefix equals one-shot",
                crc32_ieee(pattern + 1, c, 511) == 0x367ED872u);
    c = 0x12345678u;
    ok &= check("crc32 with zero length preserves the running value",
                crc32_ieee(pattern, c, 0) == c);
    ok &= check("crc32 of a single zero byte matches the reference value",
                crc32_ieee("\x00", 0, 1) == 0xD202EF8Du);

    /* --- ATA capacity ------------------------------------------------ */
    memset(id, 0, sizeof(id));
    ok &= check("identify with no LBA fields reports zero capacity",
                ata_capacity_sectors(id) == 0);

  /* LBA28 only: bits 0-15 = w[60] = 0x1234, bits 16-27 = w[61] & 0x0FFF
      = 0x0ABC -> 0x0ABC1234 */
   memset(id, 0, sizeof(id));
   id[60] = 0x1234;
   id[61] = 0x0ABC;
   ok &= check("28-bit LBA capacity is decoded from words 60-61",
               ata_capacity_sectors(id) == 0x0ABC1234ULL);

    /* 28-bit maximum */
    memset(id, 0, sizeof(id));
    id[60] = 0xFFFF;
    id[61] = 0x0FFF;
    ok &= check("28-bit LBA saturates at 2^28-1",
                ata_capacity_sectors(id) == 0x0FFFFFFFULL);

    /* LBA48 only: 0x1000056781234 (above the 32-bit boundary) */
    memset(id, 0, sizeof(id));
    id[100] = 0x1234;
    id[101] = 0x5678;
    id[102] = 0x0000;
    id[103] = 0x0001;
    ok &= check("48-bit LBA capacity is decoded from words 100-103",
                ata_capacity_sectors(id) == 0x0001000056781234ULL);

    /* 48-bit exactly 2^32 (the case the old 32-bit path corrupted) */
    memset(id, 0, sizeof(id));
    id[102] = 0x0001;
    ok &= check("48-bit LBA of 2^32 is preserved without truncation",
                ata_capacity_sectors(id) == 0x100000000ULL);

    /* both present: 48-bit wins */
    memset(id, 0, sizeof(id));
    id[60] = 0xFFFF;
    id[61] = 0x0FFF;
    id[100] = 0x0001;
    ok &= check("48-bit LBA takes precedence over 28-bit LBA",
                ata_capacity_sectors(id) == 0x0000000000000001ULL);

    /* word 61 high bits must be masked (device-specific data) */
    memset(id, 0, sizeof(id));
    id[60] = 0x0001;
    id[61] = 0xFFFF;
    ok &= check("word 61 bits above bit 12 are masked out",
                ata_capacity_sectors(id) == 0x0FFF0001ULL);
    return ok ? 0 : 1;
}
