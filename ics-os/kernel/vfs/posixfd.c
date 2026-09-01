/*
 * POSIX file descriptors and io_uring (I/O P3 + async virtio completions).
 *
 * DEX fopen/fread stay as compat. New syscalls allocate per-process fds
 * wrapping file_PCB. Cache hits and ramdisk SQEs complete inline.
 * /dev/vblk READ/WRITE/FSYNC SQEs are submitted to virtio-blk and the
 * MSI-X harvest posts CQEs. io_uring_enter honors min_complete and
 * IORING_ENTER_GETEVENTS. Ring VA is params.sq_off.user_addr (no mmap).
 */
#include "../dextypes.h"
#include "../process/process.h"
#include "../console/tty.h"
#include "../iomgr/blkcache.h"
#include "../iomgr/iosched.h"
#include "../hardware/virtio/virtio.h"
#include "../hardware/virtio/virtio_blk.h"
#include "../stdlib/time.h"
#include "posixfd.h"

extern void *malloc(unsigned int);
extern void free(void *);
extern void *memset(void *s, int c, unsigned int n);
extern void *memcpy(void *d, const void *s, unsigned int n);
extern int strcmp(const char *s, const char *t);
extern void putc(char c);
extern int vfs_setbuffer(file_PCB *handle, char *buffer, int bufsize, int mode);
extern unsigned int ticks;
extern char *userspace;
extern char *showpath(char *s);
extern int dex32_loader(char *name, char *image, char *loadaddress, int mode,
                        char *p, char *workdir, PCB386 *parent);
extern void dex32_stopints(DWORD *flags);
extern void dex32_restoreints(DWORD flags);
extern unsigned int strlen(const char *s);
extern int vfs_listdir(vfs_node *current_dir, vfs_node *buffer, int size);
extern PCB386 *sched_gethead(void);

#define SPAWN_CMDLINE 4096
#define WNOHANG    1

#define EPERM   1
#define ENOENT  2
#define EIO     5
#define ENOEXEC 8
#define EBADF   9
#define ECHILD  10
#define ENOMEM  12
#define EINVAL  22
#define EMFILE  24
#define ENOSYS  38

#define URING_MAX 64
#define URING_QUEUED  1
#define URING_WAIT_TICKS 500
#define VBLK_MAX_XFER 4096

typedef struct ics_uring ics_uring;

typedef struct {
   ics_uring *ring;
   u64 user_data;
} uring_blk_cb;

typedef struct {
   u64 off;
} fd_blk_t;

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

struct ics_uring {
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
   PCB386 *waiter;
   uring_blk_cb blkcb[URING_MAX];
};

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

static int path_is_vblk(const char *p)
{
   return p && (strcmp(p, "/dev/vblk") == 0 || strcmp(p, "vblk") == 0);
}

static fd_blk_t *fd_blk(int fd)
{
   if (!current_process || fd < 0 || fd >= FD_MAX)
      return 0;
   if (current_process->fds[fd].type != FD_BLK)
      return 0;
   return (fd_blk_t *)current_process->fds[fd].ptr;
}

static long vblk_posix_rw(int write, u64 off, void *buf, unsigned long len)
{
   u64 cap, end;
   int n;

   if (!virtio_blk_present())
      return -EIO;
   if (!buf || len == 0)
      return 0;
   if ((off | (u64)len) & (VIRTIO_BLK_SECTOR_SIZE - 1))
      return -EINVAL;
   cap = virtio_blk_sectors() * (u64)VIRTIO_BLK_SECTOR_SIZE;
   if (off >= cap)
      return 0;
   end = off + len;
   if (end > cap)
      len = (unsigned long)(cap - off);
   if (len > VBLK_MAX_XFER)
      len = VBLK_MAX_XFER;
   if (write && virtio_blk_readonly())
      return -EIO;
   n = virtio_blk_rw(write, off, buf, (u32)len);
   return n;
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

long sys_open(const char *path, int flags, int mode)
{
   int fd, vmode;
   file_PCB *fcb;
   (void)mode;
   if (!path || !current_process)
      return -EINVAL;
   if (path_is_vblk(path)) {
      fd_blk_t *b;
      if (!virtio_blk_present())
         return -ENOENT;
      if ((flags & ICSOS_O_ACCMODE) != ICSOS_O_RDONLY &&
          virtio_blk_readonly())
         return -EIO;
      fd = fd_alloc_slot();
      if (fd < 0)
         return fd;
      b = (fd_blk_t *)malloc(sizeof(fd_blk_t));
      if (!b)
         return -ENOMEM;
      b->off = 0;
      current_process->fds[fd].type = FD_BLK;
      current_process->fds[fd].ptr = b;
      return fd;
   }
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

long sys_close(int fd)
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
   if (current_process->fds[fd].type == FD_BLK) {
      fd_blk_t *b = (fd_blk_t *)current_process->fds[fd].ptr;
      current_process->fds[fd].type = FD_NONE;
      current_process->fds[fd].ptr = 0;
      if (b)
         free(b);
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
   {
      fd_blk_t *b = fd_blk(fd);
      if (b) {
         long got = vblk_posix_rw(0, b->off, buf, (unsigned long)n);
         if (got > 0)
            b->off += (u64)got;
         return got;
      }
   }
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
   {
      fd_blk_t *b = fd_blk(fd);
      if (b) {
         long got = vblk_posix_rw(1, b->off, (void *)buf, (unsigned long)n);
         if (got > 0)
            b->off += (u64)got;
         return got;
      }
   }
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
   file_PCB *f;
   fd_blk_t *b = fd_blk(fd);
   if (b) {
      u64 cap = virtio_blk_sectors() * (u64)VIRTIO_BLK_SECTOR_SIZE;
      s64 noff;
      if (whence == SEEK_SET)
         noff = off;
      else if (whence == SEEK_CUR)
         noff = (s64)b->off + off;
      else if (whence == SEEK_END)
         noff = (s64)cap + off;
      else
         return -EINVAL;
      if (noff < 0)
         return -EINVAL;
      b->off = (u64)noff;
      return (long)b->off;
   }
   f = fd_file(fd);
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
   file_PCB *f;
   fd_blk_t *b = fd_blk(fd);
   long total = 0;
   int i;

   if (b) {
      u64 off = (u64)offset;
      if (!iov || iovcnt <= 0)
         return -EINVAL;
      for (i = 0; i < iovcnt; i++) {
         long n;
         if (!iov[i].iov_base && iov[i].iov_len)
            return -EINVAL;
         if (iov[i].iov_len == 0)
            continue;
         n = vblk_posix_rw(0, off, iov[i].iov_base, iov[i].iov_len);
         if (n < 0)
            return total ? total : n;
         total += n;
         off += (u64)n;
         if ((unsigned long)n < iov[i].iov_len)
            break;
      }
      return total;
   }
   f = fd_file(fd);
   {
      long old, n;
      if (!f)
         return -EBADF;
      old = ftell(f);
      fseek(f, offset, SEEK_SET);
      n = do_iov_rw(f, iov, iovcnt, 0);
      fseek(f, old, SEEK_SET);
      return n;
   }
}

long sys_pwritev(int fd, const struct k_iovec *iov, int iovcnt, long offset)
{
   file_PCB *f;
   fd_blk_t *b = fd_blk(fd);
   long total = 0;
   int i;

   if (b) {
      u64 off = (u64)offset;
      if (!iov || iovcnt <= 0)
         return -EINVAL;
      for (i = 0; i < iovcnt; i++) {
         long n;
         if (!iov[i].iov_base && iov[i].iov_len)
            return -EINVAL;
         if (iov[i].iov_len == 0)
            continue;
         n = vblk_posix_rw(1, off, iov[i].iov_base, iov[i].iov_len);
         if (n < 0)
            return total ? total : n;
         total += n;
         off += (u64)n;
         if ((unsigned long)n < iov[i].iov_len)
            break;
      }
      return total;
   }
   f = fd_file(fd);
   {
      long old, n;
      if (!f)
         return -EBADF;
      old = ftell(f);
      fseek(f, offset, SEEK_SET);
      n = do_iov_rw(f, iov, iovcnt, 1);
      fseek(f, old, SEEK_SET);
      return n;
   }
}

long sys_fsync(int fd)
{
   file_PCB *f;
   if (fd_blk(fd))
      return virtio_blk_flush();
   f = fd_file(fd);
   if (!f)
      return -EBADF;
   if (!fflush(f))
      return -EIO;
   iomgr_request_flush();
   if (!blkcache_flush())
      return -EIO;
   return 0;
}

long sys_fstat_fd(int fd, void *statbuf)
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
   if (fd_blk(fd)) {
      memset(st, 0, sizeof(*st));
      st->st_mode = 0060000 | 0666; /* S_IFBLK */
      st->st_size = (long)(virtio_blk_sectors() * (u64)VIRTIO_BLK_SECTOR_SIZE);
      return 0;
   }
   if (!f)
      return -EBADF;
   memset(&vs, 0, sizeof(vs));
   vs.size = (int)sizeof(vs);
   if (fstat(f, &vs) < 0)
      return -EIO;
   memset(st, 0, sizeof(*st));
   /* Build tools compare (st_dev, st_ino) to remove duplicate include and
      search directories. Returning zero for every VFS object collapsed
      GCC's complete include path to its first entry. Filesystem ID plus the
      stable mounted-node identity gives each live VFS object a real key. */
   if (f->ptr) {
      st->st_dev = f->ptr->fsid;
      st->st_ino = (unsigned int)((uintptr)f->ptr >> 4);
   }
   st->st_size = vs.st_size;
   /* Report the true file type so S_ISDIR/S_ISREG work. ld's
      find_scripts_dir() checks S_ISDIR to locate its ldscripts/
      directory; a blanket S_IFREG made the check fail and ld could
      not find its default linker script. */
   st->st_mode = (f->ptr && (f->ptr->attb & FILE_DIRECTORY))
                    ? (0040000 | 0777) /* S_IFDIR */
                    : (0100000 | 0777); /* S_IFREG */
   st->st_atime = vs.st_atime;
   st->st_mtime = vs.st_mtime;
   st->st_ctime = vs.st_ctime;
   /* ICS-OS has no hard-link support; every file has exactly one link.
    * GNU binutils smart_rename() only uses rename(2) when the destination
    * has st_nlink == 1; a zero value forces its copy-fallback path. */
   st->st_nlink = 1;
   return 0;
}

void *sys_fd_file(int fd)
{
   return fd_file(fd);
}

long sys_io_uring_setup(unsigned entries, void *params)
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

static void uring_vblk_done(void *arg, int res)
{
   uring_blk_cb *cb = (uring_blk_cb *)arg;
   ics_uring *r;
   DWORD cq_tail;
   io_uring_cqe *cqe;

   if (!cb || !cb->ring)
      return;
   r = cb->ring;
   cq_tail = r->cq_tail;
   cqe = &r->cqes[cq_tail & r->cq_ring_mask];
   cqe->user_data = cb->user_data;
   cqe->res = res;
   cqe->flags = 0;
   r->cq_tail = cq_tail + 1;
   cb->ring = 0;
   if (r->waiter)
      r->waiter->waiting = 0;
}

static int uring_alloc_cb(ics_uring *r)
{
   int i;
   for (i = 0; i < URING_MAX; i++)
      if (!r->blkcb[i].ring)
         return i;
   return -1;
}

static int uring_queue_vblk(ics_uring *r, io_uring_sqe *sqe,
                            u32 type, void *buf, u32 bytes, u64 off)
{
   int i, n;
   u64 sector;

   if ((off | (u64)bytes) & (VIRTIO_BLK_SECTOR_SIZE - 1))
      return -EINVAL;
   if (!virtio_blk_present())
      return -EIO;
   i = uring_alloc_cb(r);
   if (i < 0)
      return -ENOMEM;
   r->blkcb[i].ring = r;
   r->blkcb[i].user_data = sqe->user_data;
   sector = off / VIRTIO_BLK_SECTOR_SIZE;
   n = virtio_blk_submit(type, sector, buf, bytes,
                         uring_vblk_done, &r->blkcb[i], 0);
   if (n < 0) {
      r->blkcb[i].ring = 0;
      return n;
   }
   return URING_QUEUED;
}

/* 0 = not a vblk SQE (caller runs it inline). URING_QUEUED or -errno. */
static int uring_try_vblk(ics_uring *r, io_uring_sqe *sqe)
{
   int fd;
   void *buf;
   u32 bytes, type;
   u64 off;

   if (!r || !sqe)
      return 0;
   fd = sqe->fd;
   if (!current_process || fd < 0 || fd >= FD_MAX)
      return 0;
   if (current_process->fds[fd].type != FD_BLK)
      return 0;

   if (sqe->opcode == IORING_OP_FSYNC) {
      int i, n;
      i = uring_alloc_cb(r);
      if (i < 0)
         return -ENOMEM;
      r->blkcb[i].ring = r;
      r->blkcb[i].user_data = sqe->user_data;
      n = virtio_blk_submit(VIRTIO_BLK_T_FLUSH, 0, 0, 0,
                            uring_vblk_done, &r->blkcb[i], 0);
      if (n < 0) {
         r->blkcb[i].ring = 0;
         return n;
      }
      return URING_QUEUED;
   }

   if (sqe->opcode == IORING_OP_READ || sqe->opcode == IORING_OP_WRITE) {
      buf = (void *)(uintptr)sqe->addr;
      bytes = sqe->len;
      off = sqe->off;
      if (off == ~(u64)0) {
         fd_blk_t *b = fd_blk(fd);
         off = b ? b->off : 0;
      }
      type = (sqe->opcode == IORING_OP_WRITE) ?
             VIRTIO_BLK_T_OUT : VIRTIO_BLK_T_IN;
      return uring_queue_vblk(r, sqe, type, buf, bytes, off);
   }
   if (sqe->opcode == IORING_OP_READV || sqe->opcode == IORING_OP_WRITEV) {
      const struct k_iovec *iov = (const struct k_iovec *)(uintptr)sqe->addr;
      if (!iov || sqe->len != 1)
         return 0;
      type = (sqe->opcode == IORING_OP_WRITEV) ?
             VIRTIO_BLK_T_OUT : VIRTIO_BLK_T_IN;
      return uring_queue_vblk(r, sqe, type, iov[0].iov_base,
                              (u32)iov[0].iov_len, sqe->off);
   }
   return 0;
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

long sys_io_uring_enter(int fd, unsigned to_submit, unsigned min_complete,
                       unsigned flags)
{
   ics_uring *r;
   unsigned i, done = 0;

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
      int v;

      if (head == tail)
         break;
      idx = r->sq_array[head & mask];
      if (idx > mask)
         idx &= mask;
      sqe = &r->sqes[idx];
      v = uring_try_vblk(r, sqe);
      if (v != URING_QUEUED) {
         io_uring_cqe *cqe;
         DWORD cq_tail = r->cq_tail;
         cqe = &r->cqes[cq_tail & r->cq_ring_mask];
         cqe->user_data = sqe->user_data;
         cqe->res = (v < 0) ? v : uring_do_sqe(sqe);
         cqe->flags = 0;
         r->cq_tail = cq_tail + 1;
      }
      r->sq_head = head + 1;
      done++;
   }

   if (min_complete) {
      unsigned start = ticks;
      r->waiter = current_process;
      while ((r->cq_tail - r->cq_head) < min_complete) {
         virtio_blk_harvest();
         if ((r->cq_tail - r->cq_head) >= min_complete)
            break;
         if (ticks - start > URING_WAIT_TICKS)
            break;
         cpu_idle();
      }
      r->waiter = 0;
   }
   (void)flags;
   return (int)done;
}

static int spawn_load(const char *path, const char *params)
{
   char *buf;
   DWORD size;
   DWORD id;
   char temp[255];
   char name[256];
   char cmd[SPAWN_CMDLINE];
   int i;

   if (!path)
      return -EINVAL;
   for (i = 0; path[i] && i < 255; i++)
      name[i] = path[i];
   name[i] = 0;
   if (!name[0])
      return -EINVAL;

   cmd[0] = 0;
   if (params) {
      for (i = 0; params[i] && i < SPAWN_CMDLINE - 1; i++)
         cmd[i] = params[i];
      cmd[i] = 0;
   }
   if (!cmd[0]) {
      for (i = 0; name[i] && i < SPAWN_CMDLINE - 1; i++)
         cmd[i] = name[i];
      cmd[i] = 0;
   }

   buf = (char *)vfs_mapfile(name, &size);
   if (!buf)
      return -ENOENT;

   id = dex32_loader(name, buf, userspace, 0, cmd, showpath(temp),
                     current_process);
   free(buf);
   if (!id || (int)id == -1)
      return -ENOEXEC;
   return (int)id;
}

long sys_spawn(const char *path, const char *params)
{
   return spawn_load(path, params);
}

static PCB386 *waitpid_live_child(PCB386 *parent)
{
   PCB386 *head, *p;
   if (!parent)
      return (PCB386 *)-1;
   head = sched_gethead();
   if (!head)
      return (PCB386 *)-1;
   p = head;
   do {
      if (p->owner == parent->processid && p != parent
          && !(p->status & PS_ATTB_THREAD)
          && !(p->status & PS_ATTB_UNLOADABLE))
         return p;
      p = p->next;
   } while (p && p != head);
   return (PCB386 *)-1;
}

long sys_waitpid(int pid, int *status, int options)
{
   PCB386 *me = current_process;
   int i, wpid, wst;

   if (!me)
      return -EINVAL;

   for (;;) {
      if (pid < -1)
         return -EINVAL;
      for (i = 0; i < me->waitq_n; i++) {
         if (pid == -1 || me->waitq_pid[i] == pid) {
            wpid = me->waitq_pid[i];
            wst = me->waitq_st[i];
            me->waitq_n--;
            for (; i < me->waitq_n; i++) {
               me->waitq_pid[i] = me->waitq_pid[i + 1];
               me->waitq_st[i] = me->waitq_st[i + 1];
            }
            if (status)
               *status = wst;
            return wpid;
          }
      }
      if (pid == 0)
         return -EINVAL;
      if (pid > 0) {
         PCB386 *p = ps_findprocess((DWORD)pid);
         if (p == (PCB386 *)-1) {
            if (options & WNOHANG)
               return -ECHILD;
            return -ECHILD;
         }
         if (options & WNOHANG)
            return 0;
         dex32_waitpid(pid, 0);
         continue;
      }
      /* pid == -1 */
      if (me->nlive <= 0 && me->waitq_n == 0)
         return -ECHILD;
      if (options & WNOHANG)
         return 0;
      {
         PCB386 *ch = waitpid_live_child(me);
         if (ch != (PCB386 *)-1)
            dex32_waitpid((int)ch->processid, 0);
         else
            taskswitch();
      }
   }
}

long sys_getdents(const char *path, char *ubuf, int ubuflen)
{
   vfs_node *dir;
   vfs_node *list;
   int n, i, off;

   if (!path || !ubuf || ubuflen < 2)
      return -EINVAL;
   if (path[0] == '.' && path[1] == 0 && current_process)
      dir = current_process->workdir;
   else
      dir = vfs_searchname(path);
   if (!dir)
      return -ENOENT;
   n = vfs_listdir(dir, 0, 0);
   if (n < 0)
      return -EIO;
   if (n == 0) {
      ubuf[0] = 0;
      return 1;
   }
   list = (vfs_node *)malloc((unsigned)n * sizeof(vfs_node));
   if (!list)
      return -ENOMEM;
   n = vfs_listdir(dir, list, n * (int)sizeof(vfs_node));
   off = 0;
   for (i = 0; i < n; i++) {
      const char *nm = list[i].name;
      int l;
      if (!nm)
         continue;
      l = (int)strlen(nm);
      if (off + l + 2 > ubuflen)
         break;
      memcpy(ubuf + off, nm, (unsigned)l + 1);
      off += l + 1;
   }
   ubuf[off++] = 0;
   free(list);
   return off;
}

long sys_execve(const char *path, const char *params)
{
   PCB386 *me = current_process;
   PCB386 *child;
   int id, oldpid;
   DWORD flags;

   /* Keep IRQs off until the child owns our pid so waitpid(old)
      cannot miss the steal window. createprocess restores the flags
      it sampled here (already off). */
   dex32_stopints(&flags);
   id = spawn_load(path, params);
   if (id < 0) {
      dex32_restoreints(flags);
      return id;
   }
   child = ps_findprocess((DWORD)id);
   if (child == (PCB386 *)-1) {
      dex32_restoreints(flags);
      return -EIO;
   }
   oldpid = (int)me->processid;
   child->processid = (DWORD)oldpid;
   child->owner = me->owner;
   me->processid = (DWORD)id;
   ps_dequeue(me);
   dex32_restoreints(flags);
   ps_switchto(child);
   for (;;)
      ;
   return 0;
}
