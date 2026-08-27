/*
 * POSIX file descriptors and a synchronous io_uring subset (I/O P3).
 *
 * DEX fopen/fread stay as compat. New syscalls allocate per-process fds
 * wrapping file_PCB. io_uring_setup/enter process SQEs inline (one syscall
 * per batch); completions go on the CQ. Ring memory is identity-mapped so
 * user code uses the VA in io_uring_params.sq_off.user_addr instead of mmap.
 */
#include "../dextypes.h"
#include "../process/process.h"
#include "../console/tty.h"
#include "../iomgr/blkcache.h"
#include "../iomgr/iosched.h"
#include "posixfd.h"

extern void *malloc(unsigned int);
extern void free(void *);
extern void *memset(void *s, int c, unsigned int n);
extern void *memcpy(void *d, const void *s, unsigned int n);
extern void putc(char c);
extern int vfs_setbuffer(file_PCB *handle, char *buffer, int bufsize, int mode);

#define EPERM   1
#define ENOENT  2
#define EIO     5
#define EBADF   9
#define ENOMEM  12
#define EINVAL  22
#define EMFILE  24
#define ENOSYS  38

#define URING_MAX 64

typedef struct __attribute__((packed)) {
   BYTE opcode;
   BYTE flags;
   WORD ioprio;
   int fd;
   u64 off;
   u64 addr;
   DWORD len;
   DWORD rw_flags;
   u64 user_data;
   WORD buf_index;
   WORD personality;
   DWORD splice_fd_in;
   u64 addr3;
   u64 pad2;
} io_uring_sqe;

typedef struct __attribute__((packed)) {
   u64 user_data;
   int res;
   DWORD flags;
} io_uring_cqe;

typedef struct {
   DWORD sq_head;
   DWORD sq_tail;
   DWORD sq_ring_mask;
   DWORD sq_ring_entries;
   DWORD sq_flags;
   DWORD sq_dropped;
   DWORD cq_head;
   DWORD cq_tail;
   DWORD cq_ring_mask;
   DWORD cq_ring_entries;
   DWORD cq_overflow;
   DWORD pad;
   DWORD sq_array[URING_MAX];
   io_uring_cqe cqes[URING_MAX];
   io_uring_sqe sqes[URING_MAX];
} ics_uring;

typedef struct __attribute__((packed)) {
   DWORD head, tail, ring_mask, ring_entries, flags, dropped, array, resv1;
   u64 user_addr;
} io_sqring_offsets;

typedef struct __attribute__((packed)) {
   DWORD head, tail, ring_mask, ring_entries, overflow, cqes, flags, resv1;
   u64 user_addr;
} io_cqring_offsets;

typedef struct __attribute__((packed)) {
   DWORD sq_entries;
   DWORD cq_entries;
   DWORD flags;
   DWORD sq_thread_cpu;
   DWORD sq_thread_idle;
   DWORD features;
   DWORD wq_fd;
   DWORD resv[3];
   io_sqring_offsets sq_off;
   io_cqring_offsets cq_off;
} io_uring_params;

static int fd_alloc_slot(void)
{
   int i;
   if (!current_process)
      return -EMFILE;
   for (i = 3; i < FD_MAX; i++)
      if (current_process->fds[i].type == FD_NONE)
         return i;
   return -EMFILE;
}

static file_PCB *fd_file(int fd)
{
   if (!current_process || fd < 0 || fd >= FD_MAX)
      return 0;
   if (current_process->fds[fd].type != FD_VFS)
      return 0;
   return (file_PCB *)current_process->fds[fd].ptr;
}

static tty_t *fd_istty(int fd)
{
   if (!current_process || fd < 0 || fd >= FD_MAX)
      return 0;
   if (current_process->fds[fd].type == FD_TTY)
      return (tty_t *)current_process->fds[fd].ptr;
   if (current_process->ctty && fd >= 0 && fd < 3)
      return current_process->ctty;
   return 0;
}

static int posix_to_vfs_mode(int flags)
{
   if (flags & ICSOS_O_APPEND)
      return FILE_APPEND;
   if ((flags & ICSOS_O_ACCMODE) == ICSOS_O_RDWR)
      return FILE_READWRITE;
   if (flags & ICSOS_O_WRONLY)
      return FILE_WRITE;
   return FILE_READ;
}

int sys_open(const char *path, int flags, int mode)
{
   int fd, vmode;
   file_PCB *fcb;
   (void)mode;
   if (!path || !current_process)
      return -EINVAL;
   fd = fd_alloc_slot();
   if (fd < 0)
      return fd;
   vmode = posix_to_vfs_mode(flags);
   fcb = openfilex((char *)path, vmode);
   if (!fcb)
      return -ENOENT;
   if (flags & ICSOS_O_DIRECT)
      vfs_setbuffer(fcb, 0, 0, FILE_IONBF);
   current_process->fds[fd].type = FD_VFS;
   current_process->fds[fd].ptr = fcb;
   return fd;
}

static void uring_free(ics_uring *r)
{
   if (r)
      free(r);
}

int sys_close(int fd)
{
   if (!current_process || fd < 0 || fd >= FD_MAX)
      return -EBADF;
   if (fd < 3)
      return 0;
   if (current_process->fds[fd].type == FD_VFS) {
      file_PCB *f = (file_PCB *)current_process->fds[fd].ptr;
      current_process->fds[fd].type = FD_NONE;
      current_process->fds[fd].ptr = 0;
      return fclose(f);
   }
   if (current_process->fds[fd].type == FD_URING) {
      uring_free((ics_uring *)current_process->fds[fd].ptr);
      current_process->fds[fd].type = FD_NONE;
      current_process->fds[fd].ptr = 0;
      return 0;
   }
   return -EBADF;
}

long sys_read(int fd, void *buf, long n)
{
   file_PCB *f;
   tty_t *t;
   int got;
   if (!buf || n <= 0)
      return 0;
   t = fd_istty(fd);
   if (t)
      return tty_read(t, (char *)buf, (int)n);
   f = fd_file(fd);
   if (!f)
      return -EBADF;
   got = fread((char *)buf, 1, (int)n, f);
   return got;
}

long sys_write(int fd, const void *buf, long n)
{
   file_PCB *f;
   tty_t *t;
   if (!buf || n <= 0)
      return 0;
   t = fd_istty(fd);
   if (t)
      return tty_write(t, (const char *)buf, (int)n);
   if (current_process && current_process->ctty && (fd == 1 || fd == 2))
      return tty_write(current_process->ctty, (const char *)buf, (int)n);
   f = fd_file(fd);
   if (f)
      return fwrite((char *)buf, 1, (int)n, f);
   if (buf && n > 0) {
      int i;
      for (i = 0; i < (int)n; i++)
         putc(((const char *)buf)[i]);
      return n;
   }
   return -EBADF;
}

long sys_lseek(int fd, long off, int whence)
{
   file_PCB *f = fd_file(fd);
   if (!f)
      return -EBADF;
   fseek(f, off, whence);
   return ftell(f);
}

static long do_iov_rw(file_PCB *f, const struct k_iovec *iov, int iovcnt,
                      int write)
{
   long total = 0;
   int i;
   if (!iov || iovcnt <= 0)
      return -EINVAL;
   for (i = 0; i < iovcnt; i++) {
      long n;
      if (!iov[i].iov_base && iov[i].iov_len)
         return -EINVAL;
      if (iov[i].iov_len == 0)
         continue;
      if (write)
         n = fwrite((char *)iov[i].iov_base, 1, (int)iov[i].iov_len, f);
      else
         n = fread((char *)iov[i].iov_base, 1, (int)iov[i].iov_len, f);
      if (n < 0)
         return total ? total : -EIO;
      total += n;
      if ((unsigned long)n < iov[i].iov_len)
         break;
   }
   return total;
}

long sys_preadv(int fd, const struct k_iovec *iov, int iovcnt, long offset)
{
   file_PCB *f = fd_file(fd);
   long old, n;
   if (!f)
      return -EBADF;
   old = ftell(f);
   fseek(f, offset, SEEK_SET);
   n = do_iov_rw(f, iov, iovcnt, 0);
   fseek(f, old, SEEK_SET);
   return n;
}

long sys_pwritev(int fd, const struct k_iovec *iov, int iovcnt, long offset)
{
   file_PCB *f = fd_file(fd);
   long old, n;
   if (!f)
      return -EBADF;
   old = ftell(f);
   fseek(f, offset, SEEK_SET);
   n = do_iov_rw(f, iov, iovcnt, 1);
   fseek(f, old, SEEK_SET);
   return n;
}

int sys_fsync(int fd)
{
   file_PCB *f = fd_file(fd);
   if (!f)
      return -EBADF;
   if (!fflush(f))
      return -EIO;
   iomgr_request_flush();
   if (!blkcache_flush())
      return -EIO;
   return 0;
}

int sys_fstat_fd(int fd, void *statbuf)
{
   file_PCB *f = fd_file(fd);
   vfs_stat vs;
   struct {
      unsigned int st_dev, st_ino, st_mode, st_nlink, st_uid, st_gid, st_rdev;
      long st_size;
      long st_atime, st_mtime, st_ctime;
   } *st = statbuf;
   if (!st)
      return -EINVAL;
   if (fd_istty(fd)) {
      memset(st, 0, sizeof(*st));
      st->st_mode = 0020000 | 0666; /* S_IFCHR */
      return 0;
   }
   if (!f)
      return -EBADF;
   memset(&vs, 0, sizeof(vs));
   vs.size = (int)sizeof(vs);
   if (fstat(f, &vs) < 0)
      return -EIO;
   memset(st, 0, sizeof(*st));
   st->st_size = vs.st_size;
   st->st_mode = 0100000 | 0777;
   st->st_atime = vs.st_atime;
   st->st_mtime = vs.st_mtime;
   st->st_ctime = vs.st_ctime;
   return 0;
}

void *sys_fd_file(int fd)
{
   return fd_file(fd);
}

int sys_io_uring_setup(unsigned entries, void *params)
{
   io_uring_params *p = (io_uring_params *)params;
   ics_uring *r;
   int fd, i;
   unsigned n;

   if (!p || !current_process)
      return -EINVAL;
   n = entries;
   if (n < 1)
      n = 1;
   if (n > URING_MAX)
      n = URING_MAX;
   /* round up to power of two */
   {
      unsigned p2 = 1;
      while (p2 < n)
         p2 <<= 1;
      n = p2;
   }

   fd = fd_alloc_slot();
   if (fd < 0)
      return fd;
   r = (ics_uring *)malloc(sizeof(ics_uring));
   if (!r)
      return -ENOMEM;
   memset(r, 0, sizeof(ics_uring));
   r->sq_ring_mask = n - 1;
   r->sq_ring_entries = n;
   r->cq_ring_mask = n - 1;
   r->cq_ring_entries = n;
   for (i = 0; i < (int)n; i++)
      r->sq_array[i] = (DWORD)i;

   memset(p, 0, sizeof(*p));
   p->sq_entries = n;
   p->cq_entries = n;
   p->sq_off.head = (DWORD)((uintptr)&r->sq_head - (uintptr)r);
   p->sq_off.tail = (DWORD)((uintptr)&r->sq_tail - (uintptr)r);
   p->sq_off.ring_mask = (DWORD)((uintptr)&r->sq_ring_mask - (uintptr)r);
   p->sq_off.ring_entries = (DWORD)((uintptr)&r->sq_ring_entries - (uintptr)r);
   p->sq_off.flags = (DWORD)((uintptr)&r->sq_flags - (uintptr)r);
   p->sq_off.dropped = (DWORD)((uintptr)&r->sq_dropped - (uintptr)r);
   p->sq_off.array = (DWORD)((uintptr)&r->sq_array[0] - (uintptr)r);
   p->sq_off.user_addr = (u64)(uintptr)r;
   p->cq_off.head = (DWORD)((uintptr)&r->cq_head - (uintptr)r);
   p->cq_off.tail = (DWORD)((uintptr)&r->cq_tail - (uintptr)r);
   p->cq_off.ring_mask = (DWORD)((uintptr)&r->cq_ring_mask - (uintptr)r);
   p->cq_off.ring_entries = (DWORD)((uintptr)&r->cq_ring_entries - (uintptr)r);
   p->cq_off.overflow = (DWORD)((uintptr)&r->cq_overflow - (uintptr)r);
   p->cq_off.cqes = (DWORD)((uintptr)&r->cqes[0] - (uintptr)r);
   p->cq_off.user_addr = (u64)(uintptr)r;
   p->resv[0] = (DWORD)((uintptr)&r->sqes[0] - (uintptr)r);

   current_process->fds[fd].type = FD_URING;
   current_process->fds[fd].ptr = r;
   return fd;
}

static int uring_do_sqe(io_uring_sqe *sqe)
{
   struct k_iovec iov;
   long n;
   u64 off;

   if (!sqe)
      return -EINVAL;
   switch (sqe->opcode) {
   case IORING_OP_NOP:
      return 0;
   case IORING_OP_FSYNC:
      return sys_fsync(sqe->fd);
   case IORING_OP_CLOSE:
      return sys_close(sqe->fd);
   case IORING_OP_OPENAT:
      return sys_open((const char *)(uintptr)sqe->addr,
                      (int)sqe->rw_flags, (int)sqe->len);
   case IORING_OP_READ:
   case IORING_OP_WRITE:
      iov.iov_base = (void *)(uintptr)sqe->addr;
      iov.iov_len = sqe->len;
      off = sqe->off;
      if (off == ~(u64)0) {
         if (sqe->opcode == IORING_OP_READ)
            return (int)sys_read(sqe->fd, iov.iov_base, (long)iov.iov_len);
         return (int)sys_write(sqe->fd, iov.iov_base, (long)iov.iov_len);
      }
      if (sqe->opcode == IORING_OP_READ)
         n = sys_preadv(sqe->fd, &iov, 1, (long)off);
      else
         n = sys_pwritev(sqe->fd, &iov, 1, (long)off);
      return (int)n;
   case IORING_OP_READV:
      n = sys_preadv(sqe->fd, (const struct k_iovec *)(uintptr)sqe->addr,
                     (int)sqe->len, (long)sqe->off);
      return (int)n;
   case IORING_OP_WRITEV:
      n = sys_pwritev(sqe->fd, (const struct k_iovec *)(uintptr)sqe->addr,
                      (int)sqe->len, (long)sqe->off);
      return (int)n;
   default:
      return -ENOSYS;
   }
}

int sys_io_uring_enter(int fd, unsigned to_submit, unsigned min_complete,
                       unsigned flags)
{
   ics_uring *r;
   unsigned i, done = 0;
   (void)min_complete;
   (void)flags;
   if (!current_process || fd < 0 || fd >= FD_MAX)
      return -EBADF;
   if (current_process->fds[fd].type != FD_URING)
      return -EBADF;
   r = (ics_uring *)current_process->fds[fd].ptr;
   if (!r)
      return -EBADF;

   for (i = 0; i < to_submit; i++) {
      DWORD head = r->sq_head;
      DWORD tail = r->sq_tail;
      DWORD mask = r->sq_ring_mask;
      DWORD idx;
      io_uring_sqe *sqe;
      io_uring_cqe *cqe;
      DWORD cq_tail;

      if (head == tail)
         break;
      idx = r->sq_array[head & mask];
      if (idx > mask)
         idx &= mask;
      sqe = &r->sqes[idx];
      cq_tail = r->cq_tail;
      cqe = &r->cqes[cq_tail & r->cq_ring_mask];
      cqe->user_data = sqe->user_data;
      cqe->res = uring_do_sqe(sqe);
      cqe->flags = 0;
      r->cq_tail = cq_tail + 1;
      r->sq_head = head + 1;
      done++;
   }
   return (int)done;
}
