/*
 * gccboot -- strict in-OS GCC 4.7.4 compiler-closure builder.
 *
 * A host-seeded GCC/cc1/as/ar/ld stage compiles every GCC-owned object used by
 * the ICS-OS C frontend, links /work/cc1.exe, then that rebuilt cc1 compiles
 * the GCC driver. The rebuilt driver is finally executed with -B/work to prove
 * that it selects the rebuilt compiler. The kernel console subsequently uses
 * /work/gcc.exe to rebuild and kexec ICS-OS.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <spawn.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define SEED_GCC "/icsos/apps/gcc.exe"
#define SEED_AS  "/icsos/apps/as.exe"
#define SEED_AR  "/icsos/apps/ar.exe"
#define SEED_LD  "/icsos/apps/ld.exe"
#define SRC      "/icsos/gccsrc"
#define WORK     "/work"
#define MAXARGV  192

static const char *base_flags[] = {
   "-m64", "-std=gnu89", "-w", "-nostdinc", "-nostdlib",
   "-fno-builtin", "-static", "-ffreestanding", "-fno-pie", "-fno-pic",
   "-fno-stack-protector", "-fno-asynchronous-unwind-tables",
   "-fno-strict-aliasing", "-mcmodel=large", "-mno-red-zone", 0
};

static const char *gcc_flags[] = {
   "-DIN_GCC", "-DHAVE_CONFIG_H",
   "-DBASEVER=\"4.7.4\"", "-DBUGURL=\"\"", "-DDATESTAMP=\"20260830\"",
   "-DDEVPHASE=\"\"", "-DREVISION=\"\"", "-DPKGVERSION=\"4.7.4\"",
   "-DTARGET_NAME=\"x86_64-ics-os\"",
   "-DHAVE_GAS_CFI_PERSONALITY_DIRECTIVE=1", "-DHAVE_GAS_CFI_DIRECTIVE=1",
   "-DHAVE_COMDAT_GROUP=1", "-DHAVE_GAS_SHF_MERGE=1",
   "-DHAVE_GAS_CFI_SECTIONS_DIRECTIVE=1", "-DHAVE_GAS_HIDDEN=1",
   "-DHAVE_GAS_MAX_SKIP_P2ALIGN=65535", "-DHAVE_AS_GOTOFF_IN_DATA=1",
   "-DHAVE_AS_IX86_FFREEP=1", "-DHAVE_AS_IX86_FILDQ=1",
   "-DHAVE_AS_IX86_FILDS=1", "-DHAVE_AS_IX86_REP_LOCK_PREFIX=1",
   "-DHAVE_AS_TLS=1", "-DHAVE_AS_GOTTPLTPCALL=1",
   "-DHAVE_AS_TLSDIRECT=1", "-DHAVE_AS_CFI_SECTIONS=1",
   "-DHAVE_AS_X86_CMPXCHG16B=1",
   "-I" SRC "/gen", "-I" SRC "/conf/gcc", "-I" SRC "/sdk/include",
   "-I" SRC "/up/gcc", "-I" SRC "/up/gcc/config",
   "-I" SRC "/up/gcc/config/i386", "-I" SRC "/up/libcpp",
   "-I" SRC "/up/libcpp/include", "-I" SRC "/up/libiberty",
   "-I" SRC "/conf/gmp", "-I" SRC "/conf/mpfr",
   "-I" SRC "/up/mpfr", "-I" SRC "/up/mpc/src",
   "-I" SRC "/up/libdecnumber", "-I" SRC "/up/libdecnumber/bid",
   "-I" SRC "/up/libgcc", "-I" SRC "/up/zlib",
   "-I" SRC "/up/include", 0
};

static const char *libcpp_src[] = {
   "charset.c", "directives.c", "directives-only.c", "errors.c", "expr.c",
   "files.c", "identifiers.c", "init.c", "lex.c", "line-map.c", "macro.c",
   "mkdeps.c", "pch.c", "symtab.c", "traditional.c", 0
};

static const char *libib_src[] = {
   "argv.c", "basename.c", "choose-temp.c", "concat.c", "copysign.c",
   "cplus-dem.c", "crc32.c", "dyn-string.c", "ffs.c", "fibheap.c",
   "filename_cmp.c", "floatformat.c", "hashtab.c", "hex.c", "insque.c",
   "lbasename.c", "make-relative-prefix.c", "make-temp-file.c", "md5.c",
   "mempcpy.c", "objalloc.c", "obstack.c", "partition.c", "splay-tree.c",
   "sort.c", "stack-limit.c", "stpcpy.c", "stpncpy.c", "strverscmp.c",
   "unlink-if-ordinary.c", "xatexit.c", "xexit.c", "xmalloc.c", "xmemdup.c",
   "xstrdup.c", "xstrerror.c", "xstrndup.c", "getopt.c", "getopt1.c",
   "asprintf.c", "vasprintf.c", "cp-demangle.c", "safe-ctype.c",
   "cp-demint.c", "physmem.c", "getruntime.c", "getpwd.c", "lrealpath.c", 0
};

static const char *dec_src[] = {
   "decContext.c", "decNumber.c", "bid/decimal32.c", "bid/decimal64.c",
   "bid/decimal128.c", "bid/bid2dpd_dpd2bid.c", "bid/host-ieee32.c",
   "bid/host-ieee64.c", "bid/host-ieee128.c", 0
};

static const char *zlib_src[] = {
   "adler32.c", "compress.c", "crc32.c", "deflate.c", "infback.c",
   "inffast.c", "inflate.c", "inftrees.c", "trees.c", "uncompr.c",
   "zutil.c", 0
};

static int exists(const char *path)
{
   struct stat st;
   return stat(path, &st) == 0;
}

static int valid_elf64(const char *path)
{
   int fd, n;
   unsigned char h[5];
   struct stat st;
   fd = open(path, O_RDONLY);
   if (fd < 0) return 0;
   n = (int)read(fd, h, sizeof(h));
   close(fd);
   if (stat(path, &st) != 0 || st.st_size < 64) return 0;
   return n == 5 && h[0] == 0x7f && h[1] == 'E' && h[2] == 'L' &&
          h[3] == 'F' && h[4] == 2;
}

static int run(const char *path, char *const argv[])
{
   pid_t pid = 0;
   int st = 0;
   if (posix_spawn(&pid, path, 0, 0, argv, 0) != 0)
      return -1;
   if (waitpid(pid, &st, 0) != pid)
      return -1;
   return 0;
}

static int copy_file(const char *src, const char *dst)
{
   int in, out, n;
   char buf[16384];
   in = open(src, O_RDONLY);
   if (in < 0) return -1;
   out = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0666);
   if (out < 0) { close(in); return -1; }
   while ((n = (int)read(in, buf, sizeof(buf))) > 0)
      if (write(out, buf, (size_t)n) != n) { close(in); close(out); return -1; }
   close(in);
   close(out);
   return n < 0 ? -1 : 0;
}

static int add_opts(char **argv, int n, const char **opts)
{
   int i;
   for (i = 0; opts && opts[i] && n < MAXARGV - 1; i++)
      argv[n++] = (char *)opts[i];
   return n;
}

static int compile_obj(const char *src, const char *out,
                       const char **flags, const char **extra)
{
   char *argv[MAXARGV];
   int n = 0;
   unlink(out);
   argv[n++] = SEED_GCC;
   argv[n++] = "-c";
   n = add_opts(argv, n, base_flags);
   n = add_opts(argv, n, flags);
   n = add_opts(argv, n, extra);
   argv[n++] = (char *)src;
   argv[n++] = "-o";
   argv[n++] = (char *)out;
   argv[n] = 0;
   if (run(SEED_GCC, argv) != 0 || !valid_elf64(out)) {
      printf("GCC_SELF_COMPILE_FAIL %s\n", src);
      return -1;
   }
   return 0;
}

static int archive_add(const char *archive, const char *obj)
{
   char *argv[5];
   argv[0] = SEED_AR;
   argv[1] = "rcs";
   argv[2] = (char *)archive;
   argv[3] = (char *)obj;
   argv[4] = 0;
   if (run(SEED_AR, argv) != 0 || !exists(archive)) {
      printf("GCC_SELF_ARCHIVE_FAIL %s\n", archive);
      return -1;
   }
   unlink(obj);
   return 0;
}

static int build_library(const char *tag, const char *root,
                         const char **sources, const char **flags,
                         const char **extra, const char *archive, int base_index)
{
   char src[320], obj[64];
   int i;
   unlink(archive);
   for (i = 0; sources[i]; i++) {
      sprintf(src, "%s/%s", root, sources[i]);
      sprintf(obj, WORK "/l%03d.o", base_index + i);
      printf("gccself: %s [%d] %s\n", tag, i, sources[i]);
      if (compile_obj(src, obj, flags, extra) != 0 ||
          archive_add(archive, obj) != 0)
         return -1;
   }
   printf("GCC_SELF_LIBRARY_OK %s\n", tag);
   return 0;
}

static int find_cc1_source(const char *stem, char *path)
{
   static const char *roots[] = {
      SRC "/gen/", SRC "/shims/", SRC "/up/gcc/",
      SRC "/up/gcc/config/i386/", SRC "/up/gcc/config/", 0
   };
   int i;
   for (i = 0; roots[i]; i++) {
      sprintf(path, "%s%s.c", roots[i], stem);
      if (exists(path)) return 0;
   }
   return -1;
}

static int build_cc1_objects(void)
{
   FILE *f;
   char line[256], stem[256], src[384], obj[64];
   const char *ggc_extra[] = { "-UHAVE_MMAP_ANON", "-UHAVE_MMAP_DEV_ZERO", 0 };
   int i = 0, len;
   unlink(WORK "/cc1.a");
   f = fopen(SRC "/cc1-objs.txt", "r");
   if (!f) return -1;
   while (fgets(line, sizeof(line), f)) {
      len = (int)strlen(line);
      while (len && (line[len-1] == '\n' || line[len-1] == '\r')) line[--len] = 0;
      if (len < 3 || strcmp(line + len - 2, ".o")) continue;
      strcpy(stem, line);
      stem[len-2] = 0;
      if (find_cc1_source(stem, src) != 0) {
         printf("GCC_SELF_SOURCE_FAIL %s\n", line);
         fclose(f);
         return -1;
      }
      sprintf(obj, WORK "/g%03d.o", i);
      printf("gccself: cc1 [%d] %s\n", i, line);
      if (compile_obj(src, obj, gcc_flags,
                      !strcmp(line, "ggc-page.o") ? ggc_extra : 0) != 0 ||
          archive_add(WORK "/cc1.a", obj) != 0) {
         fclose(f);
         return -1;
      }
      i++;
   }
   fclose(f);
   if (i != 349) {
      printf("GCC_SELF_OBJECT_COUNT_FAIL %d\n", i);
      return -1;
   }
   printf("GCC_SELF_OBJECTS_OK %d\n", i);
   return 0;
}

static int build_runtime(void)
{
   static const char *names[] = { "crt1", "tccsdk", "libtcc1", "posix", "setjmp", 0 };
   static const char *inc[] = { "-I" SRC "/sdk/include", 0 };
   char src[256], obj[64];
   int i;
   for (i = 0; names[i]; i++) {
      sprintf(src, SRC "/sdk/%s.c", names[i]);
      sprintf(obj, WORK "/%s.o", names[i]);
      if (compile_obj(src, obj, inc, 0) != 0) return -1;
   }
   printf("GCC_SELF_RUNTIME_OK\n");
   return 0;
}

static int link_cc1(void)
{
   char *argv[32];
   int n = 0;
   if (!exists("/icsos/seed/libmpc.a") ||
       !exists("/icsos/seed/libmpfr.a") ||
       !exists("/icsos/seed/libgmp.a")) {
      printf("GCC_SELF_LINK_FAIL missing seed libraries\n");
      return -1;
   }
   unlink(WORK "/cc1.exe");
   argv[n++] = SEED_LD;
   argv[n++] = "-T";
   argv[n++] = "/icsos/apps/ldscripts/elf_x86_64.xc";
   argv[n++] = WORK "/crt1.o";
   argv[n++] = WORK "/tccsdk.o";
   argv[n++] = WORK "/libtcc1.o";
   argv[n++] = WORK "/posix.o";
   argv[n++] = WORK "/setjmp.o";
   argv[n++] = "--start-group";
   argv[n++] = WORK "/cc1.a";
   argv[n++] = WORK "/libib.a";
   argv[n++] = WORK "/libcpp.a";
   argv[n++] = WORK "/libdec.a";
   argv[n++] = WORK "/libz.a";
   argv[n++] = "/icsos/seed/libmpc.a";
   argv[n++] = "/icsos/seed/libmpfr.a";
   argv[n++] = "/icsos/seed/libgmp.a";
   argv[n++] = "--end-group";
   argv[n++] = "-o";
   argv[n++] = WORK "/cc1.exe";
   argv[n] = 0;
   if (run(SEED_LD, argv) != 0 || !exists(WORK "/cc1.exe")) {
      printf("GCC_SELF_LINK_FAIL cc1\n");
      return -1;
   }
   printf("GCC_SELF_CC1_LINK_OK\n");
   return 0;
}

static int build_driver_with_new_cc1(void)
{
   char *ccargv[32], *asargv[6], *ldargv[16];
   int n = 0;
   const char *asmfile = "/ramdisk/gccnew.s";
   unlink(asmfile);
   ccargv[n++] = WORK "/cc1.exe";
   ccargv[n++] = "-m64";
   ccargv[n++] = "-std=gnu89";
   ccargv[n++] = "-w";
   ccargv[n++] = "-nostdinc";
   ccargv[n++] = "-fno-builtin";
   ccargv[n++] = "-ffreestanding";
   ccargv[n++] = "-fno-pie";
   ccargv[n++] = "-fno-pic";
   ccargv[n++] = "-fno-stack-protector";
   ccargv[n++] = "-fno-asynchronous-unwind-tables";
   ccargv[n++] = "-fno-strict-aliasing";
   ccargv[n++] = "-mcmodel=large";
   ccargv[n++] = "-mno-red-zone";
   ccargv[n++] = "-I" SRC "/sdk/include";
   ccargv[n++] = SRC "/gccdriver.c";
   ccargv[n++] = "-o";
   ccargv[n++] = (char *)asmfile;
   ccargv[n] = 0;
   if (run(WORK "/cc1.exe", ccargv) != 0 || !exists(asmfile)) {
      printf("GCC_SELF_STAGE2_FAIL cc1-driver\n");
      return -1;
   }
   asargv[0] = SEED_AS; asargv[1] = "--64"; asargv[2] = (char *)asmfile;
   asargv[3] = "-o"; asargv[4] = WORK "/gccdrv.o"; asargv[5] = 0;
   unlink(WORK "/gccdrv.o");
   if (run(SEED_AS, asargv) != 0 || !exists(WORK "/gccdrv.o")) {
      printf("GCC_SELF_STAGE2_FAIL as-driver\n");
      return -1;
   }
   n = 0;
   ldargv[n++] = SEED_LD;
   ldargv[n++] = "-T";
   ldargv[n++] = "/icsos/apps/ldscripts/elf_x86_64.xc";
   ldargv[n++] = WORK "/gccdrv.o";
   ldargv[n++] = WORK "/crt1.o"; ldargv[n++] = WORK "/tccsdk.o";
   ldargv[n++] = WORK "/libtcc1.o"; ldargv[n++] = WORK "/posix.o";
   ldargv[n++] = WORK "/setjmp.o"; ldargv[n++] = "-o";
   ldargv[n++] = WORK "/gcc.exe"; ldargv[n] = 0;
   unlink(WORK "/gcc.exe");
   if (run(SEED_LD, ldargv) != 0 || !exists(WORK "/gcc.exe")) {
      printf("GCC_SELF_STAGE2_FAIL link-driver\n");
      return -1;
   }
   if (copy_file(SEED_AS, WORK "/as.exe") != 0 ||
       copy_file(SEED_LD, WORK "/ld.exe") != 0) {
      printf("GCC_SELF_STAGE2_FAIL copy-tools\n");
      return -1;
   }
   printf("GCC_SELF_REBUILD_OK\n");
   return 0;
}

static int prove_rebuilt_driver(void)
{
   char *argv[32];
   int n = 0;
   unlink(WORK "/loop.o");
   argv[n++] = WORK "/gcc.exe";
   argv[n++] = "-B" WORK;
   argv[n++] = "-c";
   argv[n++] = "-m64";
   argv[n++] = "-std=gnu89";
   argv[n++] = "-w";
   argv[n++] = "-nostdinc";
   argv[n++] = "-fno-builtin";
   argv[n++] = "-ffreestanding";
   argv[n++] = "-fno-pie";
   argv[n++] = "-fno-pic";
   argv[n++] = "-mcmodel=large";
   argv[n++] = "-mno-red-zone";
   argv[n++] = "-I" SRC "/sdk/include";
   argv[n++] = SRC "/gccdriver.c";
   argv[n++] = "-o";
   argv[n++] = WORK "/loop.o";
   argv[n] = 0;
   if (run(WORK "/gcc.exe", argv) != 0 || !exists(WORK "/loop.o")) {
      printf("GCC_SELF_LOOP_FAIL\n");
      return -1;
   }
   printf("GCC_SELF_LOOP_OK\n");
   return 0;
}

int main(void)
{
   static const char *common_inc[] = {
      "-DHAVE_CONFIG_H", "-I" SRC "/sdk/include", "-I" SRC "/conf/gcc",
      "-I" SRC "/up/gcc", "-I" SRC "/up/libcpp",
      "-I" SRC "/up/libcpp/include", "-I" SRC "/up/libiberty",
      "-I" SRC "/conf/gmp", "-I" SRC "/conf/mpfr",
      "-I" SRC "/up/mpfr", "-I" SRC "/up/include", 0
   };
   static const char *dec_inc[] = {
      "-DHAVE_CONFIG_H", "-I" SRC "/up/libdecnumber",
      "-I" SRC "/up/libdecnumber/bid", "-I" SRC "/up/libgcc",
      "-I" SRC "/conf/decnumber", "-I" SRC "/sdk/include", 0
   };
   static const char *z_inc[] = { "-I" SRC "/up/zlib", "-I" SRC "/sdk/include", 0 };

   printf("GCC_SELF_BEGIN\n");
   if (!exists("/work") || !exists(SEED_GCC) || !exists(SRC "/cc1-objs.txt")) {
      printf("GCC_SELF_FAIL prerequisites\n");
      return 1;
   }
   if (build_library("libcpp", SRC "/up/libcpp", libcpp_src,
                     common_inc, 0, WORK "/libcpp.a", 0) != 0 ||
       build_library("libiberty", SRC "/up/libiberty", libib_src,
                     common_inc, 0, WORK "/libib.a", 100) != 0 ||
       build_library("libdecnumber", SRC "/up/libdecnumber", dec_src,
                     dec_inc, 0, WORK "/libdec.a", 200) != 0 ||
       build_library("zlib", SRC "/up/zlib", zlib_src,
                     z_inc, 0, WORK "/libz.a", 300) != 0 ||
       build_cc1_objects() != 0 || build_runtime() != 0 || link_cc1() != 0 ||
       build_driver_with_new_cc1() != 0 || prove_rebuilt_driver() != 0) {
      printf("GCC_SELF_FAIL build\n");
      return 1;
   }
   printf("GCC_SELF_PASS\n");
   return 0;
}
