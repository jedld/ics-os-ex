/*
 * gccdriver -- a minimal in-OS "gcc" front-end for the GCC self-host.
 *
 * This is the gcc DRIVER step of the self-host chain (see docs/gcc-selfhost.md).
 * It is a thin C program that composes the REAL GCC C frontend (cc1, built in-OS)
 * and the REAL GNU binutils backend (as, ld, built in-OS) into a single
 * `gcc x.c -o x` compile+link, using the SDK's posix_spawn/waitpid to drive
 * each phase in turn.  The actual C compilation is still performed by the real
 * cc1, so this is a genuine GCC toolchain, not a reimplementation of the
 * compiler -- only the driver glue is hand-written.
 *
 * Why a hand-written driver: the pruned GCC 4.7.4 source kept in references/
 * retains only the cc1 frontend objects; gcc.cc / options.cc / gcc.opt (the
 * upstream driver) were removed, and regenerating options.h requires running
 * genopt (a large extra build).  A thin driver is the standard approach for
 * bare-metal / embedded toolchains and is exactly the glue the self-host
 * needs: one `gcc` that spawns cc1/as/ld.
 *
 * The kernel does not propagate a child's exit status through waitpid (see
 * bintest.c), so the driver verifies each phase by checking its OWN output
 * file exists and is a plausible ELF -- the same strategy bintest uses.
 *
 * The tool ELFs (cc1/as/ld) are loaded from TOOLDIR (the CD) by the kernel exec
 * path, which is safe.  The files the children READ as input (the .c source, the
 * intermediate .s/.o, and the SDK runtime .o's fed to ld) must live on /ramdisk,
 * because a spawned child reading the CD mid-run is flaky (the proven
 * bintest/selfhost pattern).  The console `gccdrv` command stages them.
 *
 * Usage (enough to build+link a C program, and to grow toward the self-host):
 *   gcc [opts] in.c -o out            compile+link  -> runnable ELF64
 *   gcc -c [opts] in.c -o out.o       compile+assemble -> relocatable object
 *   gcc [opts] in.s -o out            assemble+link
 *
 * Forwarded to cc1: -O0 -O1 -O2 -O3 -Os -g -I<dir> -D<def> -U<def> -std=<s> -f<flag>
 * Forwarded to ld:   -L<dir> -l<name> -static -Wl,<flag>
 * Driver-only:       -c  -o<n>  -nostdlib
 * When linking (no -c), the SDK runtime -- the ICS-OS "libc"
 * (crt1/tccsdk/libtcc1/posix/setjmp .o's in RTDIR) -- is linked in
 * automatically unless -nostdlib is given.
 */
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <spawn.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define TOOLDIR  "/icsos/apps"   /* cc1/as/ld ELFs (loaded by the kernel exec path) */
#define RTDIR    "/ramdisk"      /* SDK runtime .o's + intermediates (child-readable) */
#define CC1      TOOLDIR "/cc1.exe"
#define AS       TOOLDIR "/as.exe"
#define LD       TOOLDIR "/ld.exe"
#define T_S      RTDIR "/.gccdrv.s"
#define T_O      RTDIR "/.gccdrv.o"

/* The SDK runtime ("libc") linked in automatically when linking (no -c). */
static const char *sdkrt[] = {
   RTDIR "/crt1.o",
   RTDIR "/tccsdk.o",
   RTDIR "/libtcc1.o",
   RTDIR "/posix.o",
   RTDIR "/setjmp.o",
   0
};

#define MAXOPTS 48
static char *cc1optargv[MAXOPTS]; static int cc1nopts = 0;
static char *ldoptargv[MAXOPTS];  static int ldnopts  = 0;

static void die(const char *phase)
{
   printf("GCC_DRV_FAIL %s\n", phase);
   _exit(1);
}

/* Run tool (path + NULL-terminated argv) and wait. Returns 0 on spawn+wait ok. */
static int run_tool(const char *path, char *const argv[])
{
   pid_t pid = 0;
   int st = 0;
   int r;
   r = posix_spawn(&pid, path, 0, 0, argv, 0);
   if (r != 0) {
      printf("gccdriver: spawn %s failed r=%d\n", path, r);
      return -1;
   }
   /* Wait before printing: the child shares the serial console, and the SDK
      printf is char-at-a-time, so printing while the child runs would
      interleave the two streams into an unreadable blob. */
   if (waitpid(pid, &st, 0) != pid) {
      printf("gccdriver: waitpid %s failed\n", path);
      return -1;
   }
   return 0;
}

/* Verify path exists, is non-empty, and (if wantelf) starts with the ELF magic.
   The child's final file flush can land just after waitpid returns, so the
   header read is retried briefly until the magic is stable. */
static int check_out(const char *path, int wantelf)
{
   int fd, n, i, try;
   char buf[8];
   long sz;
   for (try = 0; try < 20; try++) {
      fd = open(path, O_RDONLY);
      if (fd < 0) {
         if (try == 0) printf("gccdriver: missing output %s\n", path);
         return -1;
      }
      sz = lseek(fd, 0, SEEK_END);
      lseek(fd, 0, SEEK_SET);
      n = (int)read(fd, buf, sizeof(buf));
      close(fd);
      if (sz < 1 || n < 1) {
         if (try == 0) printf("gccdriver: empty output %s (sz=%ld)\n", path, (long)sz);
         return -1;
      }
      if (!wantelf) return (int)sz;
      if (n >= 4 && buf[0] == (char)0x7f && buf[1] == 'E' &&
          buf[2] == 'L' && buf[3] == 'F')
         return (int)sz;
      if (try == 19) {
         printf("gccdriver: not an ELF: %s (sz=%ld n=%d)\n", path, (long)sz, n);
         return -1;
      }
      for (i = 0; i < 20000; i++) { }
   }
   return (int)sz;
}

int main(int argc, char **argv)
{
   static char out[256];
   static char inc[256];
   static char objsrc[256];      /* object fed to ld (temp) or the -c output */
   static char *cc1argv[64];
   static char *asargv[16];
   static char *ldargv[96];
   int i, k;
   int compile_only = 0;
   int nostdlib = 0;
   int have_in = 0;
   int have_out = 0;
   int is_s = 0;

   /* ---- parse the gcc command line ---- */
   for (i = 1; i < argc; i++) {
      char *a = argv[i];
      size_t L;
      if (a[0] != '-') {
         L = strlen(a);
         if (L >= 2 && a[L-2] == '.') {
            if (a[L-1] == 'c')      { strcpy(inc, a); have_in = 1; }
            else if (a[L-1] == 's') { strcpy(inc, a); have_in = 1; is_s = 1; }
         }
         continue;
      }
      if (!strcmp(a, "-c"))         { compile_only = 1; continue; }
      if (!strcmp(a, "-o"))         { if (i+1 < argc) { strcpy(out, argv[++i]); have_out = 1; } continue; }
      if (!strcmp(a, "-nostdlib"))  { nostdlib = 1; continue; }
      if (!strcmp(a, "-g"))         { if (cc1nopts < MAXOPTS) cc1optargv[cc1nopts++] = a; continue; }
      if (a[1] == 'O' || a[1] == 'f') { if (cc1nopts < MAXOPTS) cc1optargv[cc1nopts++] = a; continue; }
      if (strncmp(a, "-std=", 5) == 0) { if (cc1nopts < MAXOPTS) cc1optargv[cc1nopts++] = a; continue; }
      if (a[1] == 'I' || a[1] == 'D' || a[1] == 'U') {
         if (a[2] == 0) {   /* separate form: -I dir */
            if (i+1 < argc) {
               if (cc1nopts < MAXOPTS) cc1optargv[cc1nopts++] = a;
               if (cc1nopts < MAXOPTS) cc1optargv[cc1nopts++] = argv[++i];
            }
         } else if (cc1nopts < MAXOPTS) {
            cc1optargv[cc1nopts++] = a;
         }
         continue;
      }
      if (a[1] == 'L' || a[1] == 'l') {
         if (a[2] == 0) {   /* separate form: -L dir */
            if (i+1 < argc) {
               if (ldnopts < MAXOPTS) ldoptargv[ldnopts++] = a;
               if (ldnopts < MAXOPTS) ldoptargv[ldnopts++] = argv[++i];
            }
         } else if (ldnopts < MAXOPTS) {
            ldoptargv[ldnopts++] = a;
         }
         continue;
      }
      if (!strcmp(a, "-static"))    { if (ldnopts < MAXOPTS) ldoptargv[ldnopts++] = a; continue; }
      if (strncmp(a, "-Wl,", 4) == 0) {
         /* forward the tail to ld as a single arg (single-flag case) */
         if (ldnopts < MAXOPTS) ldoptargv[ldnopts++] = a + 4;
         continue;
      }
      /* unknown driver flag: drop (e.g. -Wall, -w, -m64) */
   }

   if (!have_in) die("parse: no input file");

   /* default output name */
   if (!have_out) {
      strcpy(out, compile_only ? "a.out.o" : "a.out");
      have_out = 1;
   }

   /* object file: for -c it IS the output; otherwise a temp fed to ld */
   if (compile_only)
      strcpy(objsrc, out);
   else
      strcpy(objsrc, T_O);

   /* ---- phase 1: cc1 (C frontend) emits assembly -- only for .c input ---- */
   if (!is_s) {
      k = 0;
      cc1argv[k++] = CC1;
      for (i = 0; i < cc1nopts; i++) cc1argv[k++] = cc1optargv[i];
      cc1argv[k++] = inc;
      cc1argv[k++] = "-o";
      cc1argv[k++] = T_S;
      cc1argv[k] = 0;
      if (run_tool(CC1, cc1argv)) die("cc1 spawn");
      if (check_out(T_S, 0) < 0) die("cc1: no asm");
      printf("gccdriver: cc1 ok\n");
   }

   /* ---- phase 2: as (GAS) assembles into an ELF64 object ---- */
   asargv[0] = AS;
   asargv[1] = "--64";
   asargv[2] = is_s ? inc : T_S;
   asargv[3] = "-o";
   asargv[4] = objsrc;
   asargv[5] = 0;
   if (run_tool(AS, asargv)) die("as spawn");
   if (check_out(objsrc, 1) < 0) die("as: no object");
   printf("gccdriver: as ok\n");

   /* ---- phase 3: ld (GNU ld) links object + SDK runtime into a runnable ELF64 ---- */
   if (!compile_only) {
      k = 0;
      ldargv[k++] = LD;
      ldargv[k++] = objsrc;
      if (!nostdlib) {
         for (i = 0; sdkrt[i]; i++) ldargv[k++] = sdkrt[i];
      }
      for (i = 0; i < ldnopts; i++) ldargv[k++] = ldoptargv[i];
      ldargv[k++] = "-o";
      ldargv[k++] = out;
      ldargv[k] = 0;
      if (run_tool(LD, ldargv)) die("ld spawn");
      if (check_out(out, 1) < 0) die("ld: no exe");
      printf("gccdriver: ld ok\n");
   }

   printf("gccdriver: wrote %s\n", out);
   printf("GCC_DRIVER_OK\n");
   return 0;
}
