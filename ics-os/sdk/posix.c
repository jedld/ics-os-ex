/*
 * POSIX helpers on top of the ICS-OS syscall libc (tccsdk.c).
 * Linked into every user program that needs a C compiler or POSIX I/O.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <sys/io_uring.h>
#include <sys/wait.h>
#include <dirent.h>
#include <pwd.h>
#include <spawn.h>
#include <signal.h>
#include <time.h>
#include <stdarg.h>

extern unsigned long dexsdk_systemcall(int function_number,long p1,long p2,
                  long p3,long p4,long p5);
extern void *malloc(size_t size);
extern void free(void *ptr);
extern void *memset(void *d, int c, size_t n);
extern void *memcpy(void *d, const void *s, size_t n);
extern size_t strlen(const char *s);
extern int printf(const char *fmt, ...);
extern int vsprintf(char *buf, const char *fmt, va_list ap);
extern void charputc(char c);
extern int getchar(void);
extern void exit(int status);

#define FXN_GETCWD   0x43
#define FXN_CHDIR    0x42
#define FXN_RENAME   0xA2
#define FXN_REBOOT   0xA3
#define FXN_FSTAT    0x58
#define FXN_STAT     36
#define FXN_TIME     0x55
#define FXN_SYSREAD  0xA4
#define FXN_SYSWRITE 0xA5
#define FXN_SYSOPEN  0xA7
#define FXN_SYSCLOSE 0xA8
#define FXN_SYSLSEEK 0xA9
#define FXN_PREADV   0xAA
#define FXN_PWRITEV  0xAB
#define FXN_FSYNC    0xAC
#define FXN_URING_SETUP 0xAD
#define FXN_URING_ENTER 0xAE
#define FXN_FSTATFD  0xAF
#define FXN_FDFILE   0xB0
#define FXN_WAITPID  0xB1
#define FXN_SPAWN    0xB2
#define FXN_EXECVE   0xB3
#define FXN_GETDENTS 0xB4

static long ics_sys(int n, long a, long b, long c, long d, long e)
{
   long r = (long)dexsdk_systemcall(n, a, b, c, d, e);
   if (r < 0) {
      errno = (int)(-r);
      return -1;
   }
   return r;
}

int open(const char *path, int flags, ...)
{
   va_list ap;
   int mode = 0666;
   long r;
   va_start(ap, flags);
   if (flags & O_CREAT)
      mode = va_arg(ap, int);
   va_end(ap);
   r = ics_sys(FXN_SYSOPEN, (long)path, flags, mode, 0, 0);
   return (int)r;
}

int creat(const char *path, int mode)
{
   return open(path, O_WRONLY | O_CREAT | O_TRUNC, mode);
}

int close(int fd)
{
   if (fd >= 0 && fd < 3)
      return 0;
   return (int)ics_sys(FXN_SYSCLOSE, fd, 0, 0, 0, 0);
}

ssize_t read(int fd, void *buf, size_t n)
{
   return (ssize_t)ics_sys(FXN_SYSREAD, fd, (long)buf, (long)n, 0, 0);
}

ssize_t write(int fd, const void *buf, size_t n)
{
   return (ssize_t)ics_sys(FXN_SYSWRITE, fd, (long)buf, (long)n, 0, 0);
}

long lseek(int fd, long off, int whence)
{
   return ics_sys(FXN_SYSLSEEK, fd, off, whence, 0, 0);
}

ssize_t preadv(int fd, const struct iovec *iov, int iovcnt, off_t offset)
{
   return (ssize_t)ics_sys(FXN_PREADV, fd, (long)iov, iovcnt, (long)offset, 0);
}

ssize_t pwritev(int fd, const struct iovec *iov, int iovcnt, off_t offset)
{
   return (ssize_t)ics_sys(FXN_PWRITEV, fd, (long)iov, iovcnt, (long)offset, 0);
}

ssize_t pread(int fd, void *buf, size_t n, off_t offset)
{
   struct iovec iov;
   iov.iov_base = buf;
   iov.iov_len = n;
   return preadv(fd, &iov, 1, offset);
}

ssize_t pwrite(int fd, const void *buf, size_t n, off_t offset)
{
   struct iovec iov;
   iov.iov_base = (void *)buf;
   iov.iov_len = n;
   return pwritev(fd, &iov, 1, offset);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt)
{
   long off = lseek(fd, 0, SEEK_CUR);
   ssize_t n;
   if (off < 0)
      return -1;
   n = preadv(fd, iov, iovcnt, off);
   if (n > 0)
      lseek(fd, off + (long)n, SEEK_SET);
   return n;
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
{
   long off = lseek(fd, 0, SEEK_CUR);
   ssize_t n;
   if (off < 0)
      return -1;
   n = pwritev(fd, iov, iovcnt, off);
   if (n > 0)
      lseek(fd, off + (long)n, SEEK_SET);
   return n;
}

int fsync(int fd)
{
   return (int)ics_sys(FXN_FSYNC, fd, 0, 0, 0, 0);
}

FILE *fdopen(int fd, const char *mode)
{
   long p;
   (void)mode;
   if (fd == 0) return stdin;
   if (fd == 1) return stdout;
   if (fd == 2) return stderr;
   /* Kernel file_PCB* so DEX fwrite/fclose still work (TinyCC ELF output). */
   p = (long)dexsdk_systemcall(FXN_FDFILE, fd, 0, 0, 0, 0);
   return p ? (FILE *)p : 0;
}

int unlink(const char *path)
{
   return remove((char *)path);
}

int rmdir(const char *path)
{
   return remove((char *)path);
}

char *getcwd(char *buf, size_t size)
{
   static char tmp[256];
   if (!buf) {
      buf = tmp;
      size = sizeof(tmp);
   }
   if (!dexsdk_systemcall(FXN_GETCWD, (long)buf, (int)size, 0, 0, 0))
      return 0;
   return buf;
}

int chdir(const char *path)
{
   return dexsdk_systemcall(FXN_CHDIR, (long)path, 0, 0, 0, 0) ? 0 : -1;
}

int isatty(int fd)
{
   return (fd == 0 || fd == 1 || fd == 2);
}

int access(const char *path, int mode)
{
   FILE *f = fopen(path, "r");
   (void)mode;
   if (!f) {
      errno = 2;
      return -1;
   }
   fclose(f);
   return 0;
}

int chmod(const char *path, mode_t mode)
{
   (void)path;
   (void)mode;
   return 0;
}

int rename(const char *oldpath, const char *newpath)
{
   int r = dexsdk_systemcall(FXN_RENAME, (long)oldpath, (long)newpath, 0, 0, 0);
   return r ? 0 : -1;
}

void *mmap(void *addr, size_t length, int prot, int flags, int fd, long offset)
{
   void *p;
   ssize_t n;
   (void)addr;
   (void)prot;

   if ((flags & MAP_ANONYMOUS) || fd < 0)
      return malloc(length);

   p = malloc(length);
   if (!p)
      return MAP_FAILED;
   n = pread(fd, p, length, offset);
   if (n < 0) {
      free(p);
      return MAP_FAILED;
   }
   if ((size_t)n < length)
      memset((char *)p + n, 0, length - n);
   return p;
}

int munmap(void *addr, size_t length)
{
   (void)length;
   free(addr);
   return 0;
}

int mprotect(void *addr, size_t len, int prot)
{
   (void)addr; (void)len; (void)prot;
   return 0;
}

double ldexp(double x, int exp)
{
   while (exp > 0) { x *= 2.0; exp--; }
   while (exp < 0) { x /= 2.0; exp++; }
   return x;
}

long long strtoll(const char *nptr, char **endptr, int base)
{
   return (long long)strtol(nptr, endptr, base);
}

unsigned long long strtoull(const char *nptr, char **endptr, int base)
{
   return (unsigned long long)strtoul(nptr, endptr, base);
}

int sscanf(const char *str, const char *fmt, ...)
{
   /* Minimal: "%d.%d" used by tcc_new() for TCC_VERSION. */
   va_list ap;
   int *a, *b;
   (void)fmt;
   va_start(ap, fmt);
   a = va_arg(ap, int *);
   b = va_arg(ap, int *);
   va_end(ap);
   *a = 0; *b = 0;
   if (str) {
      *a = atoi(str);
      while (*str && *str != '.') str++;
      if (*str == '.') *b = atoi(str + 1);
   }
   return 2;
}

int execvp(const char *file, char *const argv[])
{
   char path[256];
   int i, n;

   if (!file || !file[0]) {
      errno = ENOENT;
      return -1;
   }
   for (i = 0; file[i]; i++)
      if (file[i] == '/')
         return execv(file, argv);

   n = 0;
   memcpy(path, "/icsos/apps/", 12);
   n = 12;
   for (i = 0; file[i] && n < 255; i++)
      path[n++] = file[i];
   path[n] = 0;
   return execv(path, argv);
}

static void posix_join_argv(char *dst, int max, char *const argv[])
{
   int n = 0, i;

   dst[0] = 0;
   if (!argv || max <= 1)
      return;
   for (i = 0; argv[i]; i++) {
      int l = (int)strlen(argv[i]);
      if (n && n + 1 < max)
         dst[n++] = ' ';
      if (n + l >= max)
         l = max - n - 1;
      if (l <= 0)
         break;
      memcpy(dst + n, argv[i], (size_t)l);
      n += l;
      dst[n] = 0;
      if (n >= max - 1)
         break;
   }
}

pid_t waitpid(pid_t pid, int *status, int options)
{
   return (pid_t)ics_sys(FXN_WAITPID, pid, (long)status, options, 0, 0);
}

int posix_spawn(pid_t *pid, const char *path,
                const posix_spawn_file_actions_t *file_actions,
                const posix_spawnattr_t *attrp,
                char *const argv[], char *const envp[])
{
   char cmd[1024];
   long r;

   (void)file_actions;
   (void)attrp;
   (void)envp;
   if (!path) {
      errno = EINVAL;
      return EINVAL;
   }
   posix_join_argv(cmd, (int)sizeof(cmd), argv);
   if (!cmd[0]) {
      int n = (int)strlen(path);
      if (n >= (int)sizeof(cmd))
         n = (int)sizeof(cmd) - 1;
      memcpy(cmd, path, (size_t)n);
      cmd[n] = 0;
   }
   r = ics_sys(FXN_SPAWN, (long)path, (long)cmd, 0, 0, 0);
   if (r < 0)
      return errno;
   if (pid)
      *pid = (pid_t)r;
   return 0;
}

int execv(const char *path, char *const argv[])
{
   char cmd[1024];

   if (!path) {
      errno = EINVAL;
      return -1;
   }
   posix_join_argv(cmd, (int)sizeof(cmd), argv);
   if (!cmd[0]) {
      int n = (int)strlen(path);
      if (n >= (int)sizeof(cmd))
         n = (int)sizeof(cmd) - 1;
      memcpy(cmd, path, (size_t)n);
      cmd[n] = 0;
   }
   ics_sys(FXN_EXECVE, (long)path, (long)cmd, 0, 0, 0);
   return -1;
}

struct tm *localtime(const time_t *t)
{
   static struct tm tm;
   time_t v = t ? *t : 0;
   memset(&tm, 0, sizeof(tm));
   tm.tm_year = 126;
   tm.tm_mday = 1;
   (void)v;
   return &tm;
}

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
   int t;
   (void)tz;
   t = dexsdk_systemcall(FXN_TIME, 0, 0, 0, 0, 0);
   if (tv) {
      tv->tv_sec = t;
      tv->tv_usec = 0;
   }
   return 0;
}

time_t time(time_t *t)
{
   time_t v = (time_t)dexsdk_systemcall(FXN_TIME, 0, 0, 0, 0, 0);
   if (t) *t = v;
   return v;
}

clock_t clock(void)
{
   return (clock_t)time(0);
}

int fstat(int fd, struct stat *buf)
{
   if (!buf) {
      errno = 22;
      return -1;
   }
   return (int)ics_sys(FXN_FSTATFD, fd, (long)buf, 0, 0, 0);
}

int stat(const char *path, struct stat *buf)
{
   int fd, r;
   fd = open(path, O_RDONLY);
   if (fd < 0)
      return -1;
   r = fstat(fd, buf);
   close(fd);
   return r;
}

void abort(void)
{
   exit(1);
}

void *calloc(size_t nmemb, size_t size)
{
   size_t n = nmemb * size;
   void *p = malloc(n);
   if (p) memset(p, 0, n);
   return p;
}

char *strdup(const char *s)
{
   size_t n = strlen(s) + 1;
   char *p = (char *)malloc(n);
   if (p) memcpy(p, s, n);
   return p;
}

char *strerror(int errnum)
{
   (void)errnum;
   return "error";
}

int abs(int n) { return n < 0 ? -n : n; }
long labs(long n) { return n < 0 ? -n : n; }

int system(const char *cmd)
{
   (void)cmd;
   return -1;
}

void _exit(int status)
{
   exit(status);
}

static int is_digit_base(int c, int base)
{
   int v;
   if (c >= '0' && c <= '9') v = c - '0';
   else if (c >= 'a' && c <= 'z') v = c - 'a' + 10;
   else if (c >= 'A' && c <= 'Z') v = c - 'A' + 10;
   else return -1;
   return (v < base) ? v : -1;
}

unsigned long strtoul(const char *nptr, char **endptr, int base)
{
   const char *s = nptr;
   unsigned long acc = 0;
   int v, neg = 0;
   while (isspace((unsigned char)*s)) s++;
   if (*s == '+' || *s == '-') {
      neg = (*s == '-');
      s++;
   }
   if (base == 0) {
      if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) { base = 16; s += 2; }
      else if (s[0] == '0') base = 8;
      else base = 10;
   } else if (base == 16 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
      s += 2;
   }
   while ((v = is_digit_base((unsigned char)*s, base)) >= 0) {
      acc = acc * (unsigned long)base + (unsigned long)v;
      s++;
   }
   if (endptr) *endptr = (char *)s;
   return neg ? (unsigned long)(-(long)acc) : acc;
}

long strtol(const char *nptr, char **endptr, int base)
{
   return (long)strtoul(nptr, endptr, base);
}

long atol(const char *s)
{
   return strtol(s, 0, 10);
}

double strtod(const char *nptr, char **endptr)
{
   const char *s = nptr;
   int neg = 0;
   double acc = 0.0, frac = 0.0, div = 1.0;
   while (isspace((unsigned char)*s)) s++;
   if (*s == '+' || *s == '-') { neg = (*s == '-'); s++; }
   while (*s >= '0' && *s <= '9') {
      acc = acc * 10.0 + (*s - '0');
      s++;
   }
   if (*s == '.') {
      s++;
      while (*s >= '0' && *s <= '9') {
         div *= 10.0;
         frac = frac * 10.0 + (*s - '0');
         s++;
      }
      acc += frac / div;
   }
   if (*s == 'e' || *s == 'E') {
      int e = 0, eneg = 0;
      s++;
      if (*s == '+' || *s == '-') { eneg = (*s == '-'); s++; }
      while (*s >= '0' && *s <= '9') { e = e * 10 + (*s - '0'); s++; }
      if (eneg) while (e--) acc /= 10.0;
      else while (e--) acc *= 10.0;
   }
   if (endptr) *endptr = (char *)s;
   return neg ? -acc : acc;
}

float strtof(const char *nptr, char **endptr)
{
   return (float)strtod(nptr, endptr);
}

long double strtold(const char *nptr, char **endptr)
{
   return (long double)strtod(nptr, endptr);
}

double fabs(double x) { return x < 0 ? -x : x; }
double floor(double x)
{
   long i = (long)x;
   if (x < 0 && (double)i != x) i--;
   return (double)i;
}
double ceil(double x)
{
   long i = (long)x;
   if (x > 0 && (double)i != x) i++;
   return (double)i;
}

int isspace(int c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v'; }
int isdigit(int c) { return c >= '0' && c <= '9'; }
int isalpha(int c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
int isalnum(int c) { return isalpha(c) || isdigit(c); }
int isxdigit(int c) { return isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }
int isupper(int c) { return c >= 'A' && c <= 'Z'; }
int islower(int c) { return c >= 'a' && c <= 'z'; }
int isprint(int c) { return c >= 0x20 && c < 0x7f; }
int isgraph(int c) { return c > 0x20 && c < 0x7f; }
int iscntrl(int c) { return (unsigned)c < 0x20 || c == 0x7f; }
int ispunct(int c) { return isprint(c) && !isalnum(c) && !isspace(c); }
int tolower(int c) { return isupper(c) ? c + 32 : c; }
int toupper(int c) { return islower(c) ? c - 32 : c; }

int puts(const char *s)
{
   printf("%s\n", s);
   return 0;
}

int putchar(int c)
{
   charputc((char)c);
   return c;
}

void rewind(FILE *f)
{
   fseek(f, 0, SEEK_SET);
}

int fprintf(FILE *f, const char *fmt, ...)
{
   va_list ap;
   int r;
   va_start(ap, fmt);
   r = vfprintf(f, fmt, ap);
   va_end(ap);
   return r;
}

int vfprintf(FILE *f, const char *fmt, va_list ap)
{
   char buf[4096];
   int n = vsprintf(buf, fmt, ap);
   if (n < 0) return n;
   if (f == stdout || f == stderr) {
      printf("%s", buf);
      return n;
   }
   return fwrite(buf, 1, n, f);
}

int snprintf(char *buf, size_t n, const char *fmt, ...)
{
   va_list ap;
   int r;
   va_start(ap, fmt);
   r = vsnprintf(buf, n, fmt, ap);
   va_end(ap);
   return r;
}

int vsnprintf(char *buf, size_t n, const char *fmt, va_list ap)
{
   char tmp[4096];
   int r = vsprintf(tmp, fmt, ap);
   if (!buf || n == 0) return r;
   if (r < 0) return r;
   if ((size_t)r >= n) {
      memcpy(buf, tmp, n - 1);
      buf[n - 1] = 0;
   } else {
      memcpy(buf, tmp, (size_t)r + 1);
   }
   return r;
}

static void qsort_swap(char *a, char *b, size_t n)
{
   while (n--) {
      char t = *a;
      *a++ = *b;
      *b++ = t;
   }
}

static void qsort_rec(char *base, int left, int right, size_t size,
                      int (*cmp)(const void *, const void *))
{
   int i, last;
   if (left >= right) return;
   qsort_swap(base + left * size, base + ((left + right) / 2) * size, size);
   last = left;
   for (i = left + 1; i <= right; i++) {
      if (cmp(base + i * size, base + left * size) < 0) {
         last++;
         qsort_swap(base + last * size, base + i * size, size);
      }
   }
   qsort_swap(base + left * size, base + last * size, size);
   qsort_rec(base, left, last - 1, size, cmp);
   qsort_rec(base, last + 1, right, size, cmp);
}

void qsort(void *base, size_t nmemb, size_t size,
           int (*compar)(const void *, const void *))
{
   if (nmemb < 2 || size == 0) return;
   qsort_rec((char *)base, 0, (int)nmemb - 1, size, compar);
}

sighandler_t signal(int sig, sighandler_t handler)
{
   (void)sig;
   return handler;
}

int fseek(FILE *f, long off, int whence)
{
   dexsdk_systemcall(0x41, (long)f, (long)off, whence, 0, 0);
   return 0;
}

char *getenv(const char *name)
{
   static char buf[512];
   if (!dexsdk_systemcall(0x9F, (long)name, (long)buf, 0, 0, 0))
      return 0;
   return buf;
}

int machine_reboot(void)
{
   return dexsdk_systemcall(FXN_REBOOT, 0, 0, 0, 0, 0);
}

int io_uring_setup(unsigned entries, struct io_uring_params *p)
{
   return (int)ics_sys(FXN_URING_SETUP, (long)entries, (long)p, 0, 0, 0);
}

int io_uring_enter(int fd, unsigned to_submit, unsigned min_complete, unsigned flags)
{
   return (int)ics_sys(FXN_URING_ENTER, fd, (long)to_submit,
                       (long)min_complete, (long)flags, 0);
}

int io_uring_queue_init(unsigned entries, struct io_uring *ring, unsigned flags)
{
   struct io_uring_params p;
   char *base;
   int fd;
   if (!ring)
      return -1;
   memset(&p, 0, sizeof(p));
   p.flags = flags;
   fd = io_uring_setup(entries, &p);
   if (fd < 0)
      return -1;
   base = (char *)(unsigned long)p.sq_off.user_addr;
   ring->ring_fd = fd;
   ring->sq_entries = p.sq_entries;
   ring->cq_entries = p.cq_entries;
   ring->sq_head = (uint32_t *)(base + p.sq_off.head);
   ring->sq_tail = (uint32_t *)(base + p.sq_off.tail);
   ring->sq_mask = (uint32_t *)(base + p.sq_off.ring_mask);
   ring->sq_array = (uint32_t *)(base + p.sq_off.array);
   ring->sqes = (struct io_uring_sqe *)(base + p.resv[0]);
   ring->cq_head = (uint32_t *)(base + p.cq_off.head);
   ring->cq_tail = (uint32_t *)(base + p.cq_off.tail);
   ring->cq_mask = (uint32_t *)(base + p.cq_off.ring_mask);
   ring->cqes = (struct io_uring_cqe *)(base + p.cq_off.cqes);
   return 0;
}

void io_uring_queue_exit(struct io_uring *ring)
{
   if (ring && ring->ring_fd >= 0)
      close(ring->ring_fd);
}

struct io_uring_sqe *io_uring_get_sqe(struct io_uring *ring)
{
   unsigned tail, head, mask, idx;
   if (!ring)
      return 0;
   tail = *ring->sq_tail;
   head = *ring->sq_head;
   mask = *ring->sq_mask;
   if (tail - head >= ring->sq_entries)
      return 0;
   idx = ring->sq_array[tail & mask];
   *ring->sq_tail = tail + 1;
   memset(&ring->sqes[idx], 0, sizeof(struct io_uring_sqe));
   return &ring->sqes[idx];
}

int io_uring_submit(struct io_uring *ring)
{
   unsigned head, tail;
   if (!ring)
      return -1;
   head = *ring->sq_head;
   tail = *ring->sq_tail;
   if (tail == head)
      return 0;
   return io_uring_enter(ring->ring_fd, tail - head, 0, 0);
}

int io_uring_submit_and_wait(struct io_uring *ring, unsigned wait_nr)
{
   unsigned head, tail;
   if (!ring)
      return -1;
   head = *ring->sq_head;
   tail = *ring->sq_tail;
   return io_uring_enter(ring->ring_fd, tail - head, wait_nr,
                         IORING_ENTER_GETEVENTS);
}

int io_uring_wait_cqe(struct io_uring *ring, struct io_uring_cqe **cqe)
{
   unsigned head, tail, mask;
   if (!ring || !cqe)
      return -1;
   head = *ring->cq_head;
   tail = *ring->cq_tail;
   if (head == tail) {
      if (io_uring_enter(ring->ring_fd, 0, 1, IORING_ENTER_GETEVENTS) < 0)
         return -1;
      head = *ring->cq_head;
      tail = *ring->cq_tail;
      if (head == tail)
         return -1;
   }
   mask = *ring->cq_mask;
   *cqe = &ring->cqes[head & mask];
   return 0;
}

void io_uring_cqe_seen(struct io_uring *ring, struct io_uring_cqe *cqe)
{
   (void)cqe;
   if (ring)
      *ring->cq_head = *ring->cq_head + 1;
}

static char *ics_env_empty[] = { 0 };
char **environ = ics_env_empty;

DIR *opendir(const char *path)
{
   DIR *d;
   char *buf;
   long n;

   if (!path) {
      errno = EINVAL;
      return 0;
   }
   buf = (char *)malloc(4096);
   if (!buf)
      return 0;
   n = ics_sys(FXN_GETDENTS, (long)path, (long)buf, 4096, 0, 0);
   if (n < 0) {
      free(buf);
      return 0;
   }
   d = (DIR *)malloc(sizeof(DIR));
   if (!d) {
      free(buf);
      return 0;
   }
   d->packed = buf;
   d->off = 0;
   return d;
}

struct dirent *readdir(DIR *dir)
{
   const char *s;
   int n;

   if (!dir || !dir->packed)
      return 0;
   s = dir->packed + dir->off;
   if (s[0] == 0)
      return 0;
   n = (int)strlen(s);
   dir->de.d_ino = 1;
   if (n > 255)
      n = 255;
   memcpy(dir->de.d_name, s, (size_t)n);
   dir->de.d_name[n] = 0;
   dir->off += (int)strlen(s) + 1;
   return &dir->de;
}

int closedir(DIR *dir)
{
   if (!dir)
      return 0;
   free(dir->packed);
   free(dir);
   return 0;
}

int umask(int mask)
{
   (void)mask;
   return 022;
}

int getuid(void) { return 0; }
int geteuid(void) { return 0; }
int getgid(void) { return 0; }
int getegid(void) { return 0; }

int dup2(int oldfd, int newfd)
{
   (void)oldfd;
   (void)newfd;
   errno = ENOSYS;
   return -1;
}

void perror(const char *s)
{
   if (s && s[0])
      printf("%s: errno=%d\n", s, errno);
   else
      printf("errno=%d\n", errno);
}

int pipe(int fd[2])
{
   (void)fd;
   errno = ENOSYS;
   return -1;
}

int flock(int fd, int op)
{
   (void)fd;
   (void)op;
   return 0;
}

int mkstemp(char *template)
{
   static int n;
   int i, len, fd;
   if (!template)
      return -1;
   len = (int)strlen(template);
   n++;
   for (i = len - 1; i >= 0 && i >= len - 6; i--) {
      if (template[i] == 'X')
         template[i] = '0' + (n % 10);
      n /= 10;
      if (n == 0)
         n = 1;
   }
   fd = open(template, O_RDWR | O_CREAT | O_TRUNC, 0600);
   return fd;
}

struct passwd *getpwnam(const char *name)
{
   (void)name;
   return 0;
}

struct passwd *getpwuid(uid_t uid)
{
   (void)uid;
   return 0;
}

int atexit(void (*fn)(void))
{
   (void)fn;
   return 0;
}

int kill(int pid, int sig)
{
   if (pid == getpid() || pid <= 0) {
      _exit(sig ? (128 + (sig & 127)) : 1);
   }
   (void)sig;
   return 0;
}

int fcntl(int fd, int cmd, ...)
{
   (void)fd;
   (void)cmd;
   return 0;
}

int setvbuf(FILE *f, char *buf, int mode, size_t size)
{
   (void)f;
   (void)buf;
   (void)mode;
   (void)size;
   return 0;
}

int fileno(FILE *f)
{
   if (!f)
      return -1;
   if (f == stdout)
      return 1;
   if (f == stderr)
      return 2;
   if (f == stdin)
      return 0;
   return 3;
}

int putc(int c, FILE *f)
{
   return fputc(c, f);
}

int ferror(FILE *f)
{
   (void)f;
   return 0;
}

double atof(const char *s)
{
   return strtod(s, 0);
}

char *ctime(const time_t *t)
{
   static char buf[32];
   (void)t;
   strcpy(buf, "Thu Jan  1 00:00:00 1970\n");
   return buf;
}

int putenv(char *string)
{
   char *eq;
   char name[128];
   int n;
   if (!string)
      return -1;
   eq = strchr(string, '=');
   if (!eq)
      return setenv(string, "", 1);
   n = (int)(eq - string);
   if (n <= 0 || n >= (int)sizeof(name))
      return -1;
   memcpy(name, string, (size_t)n);
   name[n] = 0;
   return setenv(name, eq + 1, 1);
}

char *getlogin(void)
{
   return 0;
}

int getloadavg(double loadavg[], int nelem)
{
   int i;
   if (!loadavg || nelem <= 0)
      return -1;
   for (i = 0; i < nelem && i < 3; i++)
      loadavg[i] = 0.0;
   return i > 0 ? i : -1;
}

pid_t vfork(void)
{
   errno = ENOSYS;
   return -1;
}

/* TinyCC emits calls to __builtin_alloca after host tcc -E; gcc will not
 * let us define that name. Stage script rewrites it to icsos_alloca. */
void *icsos_alloca(unsigned long n)
{
   if (n == 0)
      n = 1;
   return malloc(n);
}

/*
 * Path / memory / resource queries. Added for the in-OS toolchain
 * (GNU binutils: libbfd getpagesize, ld realpath, libiberty pathconf /
 * sysconf / getrlimit). Userspace-only: no new syscalls.
 */
#include <sys/resource.h>
#include <sys/param.h>

#define ICS_PAGE_SIZE 4096

int getpagesize(void)
{
   return ICS_PAGE_SIZE;
}

long sysconf(int name)
{
   switch (name) {
   case _SC_PAGESIZE:
      return ICS_PAGE_SIZE;
   case _SC_CLK_TCK:
      return 100;
   case _SC_NPROCESSORS_CONF:
   case _SC_NPROCESSORS_ONLN:
      return 1;   /* conservative; no user-visible cpu count syscall */
   default:
      return -1;
   }
}

long pathconf(const char *path, int name)
{
   (void)path;
   switch (name) {
   case _PC_PATH_MAX:
      return PATH_MAX;
   case _PC_NAME_MAX:
      return NAME_MAX;
   default:
      return -1;
   }
}

/* Canonicalise a path without following symlinks beyond the final
   component (ICS-OS has no symlinks yet). Resolves "." and ".." lexically. */
char *realpath(const char *path, char *resolved)
{
   char *out, *w;
   const char *p;
   size_t len;
   int nseg, i, segs[64];
   char names[64][256];
   int abs;

   if (!path || !*path) { errno = ENOENT; return 0; }
   abs = (path[0] == '/');

   nseg = 0;
   p = path;
   while (nseg < 64) {
      while (*p == '/') p++;
      if (!*p) break;
      w = names[nseg];
      len = 0;
      while (*p && *p != '/' && len < 255) w[len++] = *p++;
      w[len] = 0;
      if (len == 0) continue;
      if (len == 1 && w[0] == '.') continue;
      if (len == 2 && w[0] == '.' && w[1] == '.') {
         if (nseg > 0) nseg--;
         continue;
      }
      names[nseg][255] = 0;
      nseg++;
   }
   if (nseg >= 64) { errno = ENAMETOOLONG; return 0; }

   out = resolved ? resolved : (char *)malloc(PATH_MAX);
   if (!out) { errno = ENOMEM; return 0; }

   if (abs) out[0] = '/'; else out[0] = 0;
   w = out + (abs ? 1 : 0);
   for (i = 0; i < nseg; i++) {
      if (w != out) *w++ = '/';
      len = strlen(names[i]);
      memcpy(w, names[i], len);
      w += len;
   }
   *w = 0;
   if (out[0] == 0) out[0] = '/';
   return out;
}

/* Per-process resource limits. ICS-OS keeps them in the SDK (no kernel
   backing); defaults are "unlimited" so tools that probe them (binutils
   stack-limit) behave. */
static struct rlimit ics_rlimits[16];
static int ics_rlimits_init = 0;

static void rlimits_init(void)
{
   int i;
   for (i = 0; i < 16; i++) {
      ics_rlimits[i].rlim_cur = RLIM_INFINITY;
      ics_rlimits[i].rlim_max = RLIM_INFINITY;
   }
   ics_rlimits[RLIMIT_NOFILE].rlim_cur = 256;
   ics_rlimits[RLIMIT_NOFILE].rlim_max = 256;
   ics_rlimits_init = 1;
}

int getrlimit(int resource, struct rlimit *rlim)
{
   if (!ics_rlimits_init) rlimits_init();
   if (!rlim || resource < 0 || resource > 15) { errno = EINVAL; return -1; }
   *rlim = ics_rlimits[resource];
   return 0;
}

int setrlimit(int resource, const struct rlimit *rlim)
{
   if (!ics_rlimits_init) rlimits_init();
   if (!rlim || resource < 0 || resource > 15) { errno = EINVAL; return -1; }
   ics_rlimits[resource] = *rlim;
   return 0;
}

/* POSIX signal sets. ICS-OS has no per-process hardware signal delivery;
   the set is tracked in the SDK so mask APIs are functional for binutils
   (libiberty sigsetmask.c, ld job control). */
static sigset_t ics_sigmask;

void sigemptyset(sigset_t *set)
{
   if (set) memset(set, 0, sizeof(*set));
}
void sigfillset(sigset_t *set)
{
   if (set) memset(set, 0xff, sizeof(*set));
}
void sigaddset(sigset_t *set, int signum)
{
   if (set && signum > 0 && signum <= 128)
      set->bits[signum / 64] |= 1UL << (signum % 64);
}
void sigdelset(sigset_t *set, int signum)
{
   if (set && signum > 0 && signum <= 128)
      set->bits[signum / 64] &= ~(1UL << (signum % 64));
}
int sigismember(const sigset_t *set, int signum)
{
   if (!set || signum <= 0 || signum > 128) return 0;
   return (set->bits[signum / 64] >> (signum % 64)) & 1;
}
int sigprocmask(int how, const sigset_t *set, sigset_t *oldset)
{
   if (oldset) *oldset = ics_sigmask;
   if (set) {
      if (how == SIG_BLOCK) {
         int i; for (i = 0; i < 4; i++) ics_sigmask.bits[i] |= set->bits[i];
      } else if (how == SIG_UNBLOCK) {
         int i; for (i = 0; i < 4; i++) ics_sigmask.bits[i] &= ~set->bits[i];
      } else { /* SIG_SETMASK */
         ics_sigmask = *set;
      }
   }
   return 0;
}
int raise(int sig)
{
   /* No delivery mechanism; a fatal self-raise terminates like kill. */
   if (sig > 0 && sig <= 128)
      _exit(128 + (sig & 127));
   return 0;
}
