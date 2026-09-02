/*
  Name: crc32.h
  Description: Standard CRC-32 (IEEE 802.3 / zlib / GPT) checksum.

  Used by the GUID Partition Table (GPT) parser to validate header and
  partition-entry blocks. Self-contained (no includes) so the exact same
  code compiles in-kernel (via kernel32.c) and in host unit tests.
*/
#ifndef ICSOS_PARTITION_CRC32_H
#define ICSOS_PARTITION_CRC32_H

/* Standard reflected CRC-32 (poly 0xEDB88320, init/xorout 0xFFFFFFFF).
   crc is the running value (0 for a fresh buffer); data/length describe
   the next chunk. The result of covering the full buffer is the checksum. */
unsigned int crc32_ieee(const void *data, unsigned int crc, unsigned int length);

#endif /* ICSOS_PARTITION_CRC32_H */
