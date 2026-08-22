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

/* Kernel FILE* handles live around 0xC0000000. Casting them to int makes
   every successful open look like fd < 0 to TinyCC. Keep a small table of
   non-negative descriptors instead. */
#define ICSOS_FD_MAX 64
#define ICSOS_FD_BASE 3

static FILE *icsos_fdtab[ICSOS_FD_MAX];

static int fd_alloc(FILE *f)
{
   int i;
   for (i = 0; i < ICSOS_FD_MAX; i++) {
      if (!icsos_fdtab[i]) {
         icsos_fdtab[i] = f;
         return ICSOS_FD_BASE + i;
      }
   }
   errno = 24; /* EMFILE */
   return -1;
}

static FILE *mapfd(int fd)
{
   if (fd == 0) return stdin;
   if (fd == 1) return stdout;
   if (fd == 2) return stderr;
   if (fd >= ICSOS_FD_BASE && fd < ICSOS_FD_BASE + ICSOS_FD_MAX)
      return icsos_fdtab[fd - ICSOS_FD_BASE];
   return 0;
}

static void fd_free(int fd)
{
   if (fd >= ICSOS_FD_BASE && fd < ICSOS_FD_BASE + ICSOS_FD_MAX)
      icsos_fdtab[fd - ICSOS_FD_BASE] = 0;
}

int open(const char *path, int flags, ...)
{
   FILE *f;
   const char *mode;
   int fd;

   if ((flags & O_ACCMODE) == O_RDONLY)
      mode = "r";
   else if (flags & O_APPEND)
      mode = "a";
   else if ((flags & O_ACCMODE) == O_RDWR)
      mode = (flags & O_CREAT) ? "w" : "r";
   else
      mode = "w";

   f = fopen(path, mode);
   if (!f) {
      errno = 2;
      return -1;
   }
   fd = fd_alloc(f);
   if (fd < 0) {
      fclose(f);
      return -1;
   }
   return fd;
}

int creat(const char *path, int mode)
{
   (void)mode;
   return open(path, O_WRONLY | O_CREAT | O_TRUNC);
}

int close(int fd)
{
   FILE *f = mapfd(fd);
   int r;
   if (!f || f == stdin || f == stdout || f == stderr)
      return 0;
   r = fclose(f);
   fd_free(fd);
   return r;
}

ssize_t read(int fd, void *buf, size_t n)
{
   FILE *f = mapfd(fd);
   int r;
   if (!f) {
      errno = 9;
      return -1;
   }
   if (f == stdin) {
      size_t i;
      char *p = (char *)buf;
      for (i = 0; i < n; i++) {
         int c = getchar();
         p[i] = (char)c;
         if (c == '\n' || c == '\r') {
            i++;
            break;
         }
      }
      return (ssize_t)i;
   }
   r = fread(buf, 1, (int)n, f);
   return r;
}

ssize_t write(int fd, const void *buf, size_t n)
{
   FILE *f = mapfd(fd);
   const char *p = (const char *)buf;
   size_t i;
   if (!f) {
      errno = 9;
      return -1;
   }
   if (f == stdout || f == stderr) {
      for (i = 0; i < n; i++)
         charputc(p[i]);
      return (ssize_t)n;
   }
   return fwrite(buf, 1, (int)n, f);
}

long lseek(int fd, long off, int whence)
{
   FILE *f = mapfd(fd);
   if (!f) {
      errno = 9;
      return -1;
   }
   fseek(f, off, whence);
   return ftell(f);
}

FILE *fdopen(int fd, const char *mode)
{
   (void)mode;
   return mapfd(fd);
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
   FILE *f;
   void *p;
   long old;
   size_t n;
   (void)addr;
   (void)prot;

   if ((flags & MAP_ANONYMOUS) || fd < 0)
      return malloc(length);

   /* Eager file-backed map: populate from the open fd (benefits from
      kernel coalesced FAT reads + block cache). */
   f = mapfd(fd);
   if (!f)
      return MAP_FAILED;
   p = malloc(length);
   if (!p)
      return MAP_FAILED;
   old = ftell(f);
   fseek(f, offset, 0 /* SEEK_SET */);
   n = fread(p, 1, length, f);
   fseek(f, old, 0);
   if (n < length)
      memset((char*)p + n, 0, length - n);
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
   (void)file; (void)argv;
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

static int fstat_file(FILE *f, struct stat *buf)
{
   struct {
      int size;
      int st_dev;
      int st_ino;
      int st_mode;
      short st_nlink;
      short st_uid;
      short st_gid;
      int st_rdev;
      int st_size;
      int st_atime;
      int st_mtime;
      int st_ctime;
   } vs;
   memset(buf, 0, sizeof(*buf));
   memset(&vs, 0, sizeof(vs));
   vs.size = (int)sizeof(vs);
   if (dexsdk_systemcall(FXN_FSTAT, (long)f, (long)&vs, 0, 0, 0) < 0)
      return -1;
   buf->st_size = vs.st_size;
   buf->st_mode = S_IFREG | 0777;
   buf->st_atime = vs.st_atime;
   buf->st_mtime = vs.st_mtime;
   buf->st_ctime = vs.st_ctime;
   return 0;
}

int stat(const char *path, struct stat *buf)
{
   FILE *f = fopen(path, "r");
   int r;
   if (!f) {
      errno = 2;
      return -1;
   }
   r = fstat_file(f, buf);
   fclose(f);
   return r;
}

int fstat(int fd, struct stat *buf)
{
   FILE *f = mapfd(fd);
   if (!f) {
      errno = 9;
      return -1;
   }
   return fstat_file(f, buf);
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
