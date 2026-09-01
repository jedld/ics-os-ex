#ifndef ICSOS_BIO_H
#define ICSOS_BIO_H

#include "../types.h"

/* Minimal Linux-style bio: one contiguous buffer, 512-or-device LBAs. */

#define BIO_READ   1
#define BIO_WRITE  2
#define BIO_FLUSH  3

#define BIO_OK     1
#define BIO_ERR    -1
#define BIO_PENDING 0

struct bio {
   int deviceid;
   int op;
   int status;
   DWORD device_generation;
   u64 sector;
   DWORD nsect;
   void *buf;
   struct bio *next;
};

int bio_submit_sync(struct bio *bio);

#endif
