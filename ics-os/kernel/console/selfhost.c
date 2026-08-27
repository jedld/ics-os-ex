// Self-host / self-compile test drivers (tccboot, kbuild, fullhost).
// Split out of console.c (single-TU build: kernel32.c #includes console.c,
// which #includes this file).  Uses only console.c's includes via console.h.
int user_execp(char *fname, DWORD mode, char *params);

/* Copy one file; print a short status. Returns 1 on success. */
static int tccboot_copy1(const char *src, const char *dst)
{
   printf("  %s -> %s\n", src, dst);
   if (fcopy((char*)src, (char*)dst) == -1) {
      printf("tccboot: copy failed: %s\n", src);
      return 0;
   }
   return 1;
}

/* Write a small text file (TCC @response list). */
static int tccboot_writefile(const char *path, const char *text)
{
   file_PCB *f;
   int n = strlen(text);
   f = openfilex((char*)path, FILE_WRITE);
   if (!f) {
      printf("tccboot: cannot create %s\n", path);
      return 0;
   }
   if (fwrite((char*)text, n, 1, f) != n) {
      printf("tccboot: write failed %s\n", path);
      fclose(f);
      return 0;
   }
   fclose(f);
   return 1;
}

/* Minimal ustar extract: regular files only, into destdir/. */
static int tccboot_untar(const char *tar, DWORD tarsize, const char *destdir)
{
   DWORD off = 0;
   while (off + 512 <= tarsize) {
      const char *hdr = tar + off;
      char name[120], path[256];
      DWORD fsize = 0, i, nwrite;
      file_PCB *f;
      const unsigned char *p;

      off += 512;
      if (hdr[0] == 0)
         break;
      if (hdr[156] == '5' || hdr[156] == '2')
         continue;
      if (hdr[156] != '0' && hdr[156] != 0)
         continue;

      memset(name, 0, sizeof(name));
      memcpy(name, hdr, 100);
      if (name[0] == '.' && name[1] == '/')
         memmove(name, name + 2, strlen(name + 2) + 1);
      if (name[0] == 0)
         continue;

      p = (const unsigned char *)(hdr + 124);
      for (i = 0; i < 11 && p[i]; i++) {
         if (p[i] >= '0' && p[i] <= '7')
            fsize = (fsize << 3) + (p[i] - '0');
      }

      {
         int bad = 0, ki;
         for (ki = 0; name[ki]; ki++) {
            char c = name[ki];
            if (c==' ' || c==':' || c=='|' || c=='*' || c=='?' ||
                c=='(' || c==')' || c=='\\')
               bad = 1;
         }
         if (bad) {
            printf("  skip invalid name %s\n", name);
            off += (fsize + 511) & ~511U;
            continue;
         }
      }

      if (off + fsize > tarsize) {
         printf("tccboot: tar truncated (%s)\n", name);
         return 0;
      }

      sprintf(path, "%s/%s", destdir, name);
      printf("  extract %s (%u)\n", path, (unsigned)fsize);
      {
         /* mkdir parents so tar members like sdk/foo.c work */
         char dir[256];
         int k;
         strcpy(dir, path);
         for (k = 1; dir[k]; k++) {
            if (dir[k] == '/') {
               dir[k] = 0;
               mkdir(dir);
               dir[k] = '/';
            }
         }
      }
      f = openfilex(path, FILE_WRITE);
      if (!f) {
         printf("tccboot: cannot create %s\n", path);
         return 0;
      }
      if (fsize) {
         nwrite = fwrite((char*)(tar + off), (int)fsize, 1, f);
         if (nwrite != fsize) {
            printf("tccboot: write failed %s\n", path);
            fclose(f);
            return 0;
         }
      }
      fclose(f);
      off += (fsize + 511) & ~511U;
   }
   return 1;
}

/* Stage via one ISO tar map (avoids many ATAPI opens). */
static int tccboot_stage(void)
{
   char *tarbuf;
   DWORD tarsize;

   printf("tccboot: loading /icsos/tccsrc.tar\n");
   tarbuf = (char*)vfs_mapfile("/icsos/tccsrc.tar", &tarsize);
   if (!tarbuf || tarsize < 512) {
      printf("tccboot: cannot map tccsrc.tar\n");
      return 0;
   }
   printf("tccboot: extracting %u bytes to /ramdisk\n", (unsigned)tarsize);
   if (!tccboot_untar(tarbuf, tarsize, "/ramdisk")) {
      free(tarbuf);
      return 0;
   }
   free(tarbuf);
   return 1;
}

static int run_tcc(const char *cc, const char *cmdline)
{
   printf("  %s\n", cmdline);
   return user_execp((char*)cc, 0, (char*)cmdline);
}

/* kernel sprintf has no %.*s — build "foo.o" from "foo.c" or "dir/foo.c". */
static void c_to_o(char *dst, const char *src)
{
   const char *slash = strrchr(src, '/');
   const char *bn = slash ? slash + 1 : src;
   int n = (int)strlen(bn);
   if (n > 2)
      n -= 1; /* drop trailing 'c' of .c */
   memcpy(dst, bn, n);
   dst[n] = 'o';
   dst[n + 1] = 0;
}

/* Rebuild TinyCC from per-file objects (ONE_SOURCE hangs in-OS). */
static int tccboot_run(void)
{
   static const char *cfiles[] = {
      "tcc.c", "libtcc.c", "tccpp.c", "tccgen.c", "tccelf.c",
      "tccrun.c", "tccasm.c", "x64gen.c", "x64lnk.c", "i386asm.c",
      0
   };
   static const char cflags[] =
      "-c -nostdlib -w -DONE_SOURCE=0 -DTCC_TARGET_X86_64 "
      "-DCONFIG_TCC_STATIC -nostdinc -I/ramdisk/tcc -I/icsos/include -I/icsos/tcc1";
   char cmd[768], obj[64];
   int i;
   const char *cc = "/icsos/apps/tcc.exe";

   if (!tccboot_stage()) {
      printf("TCCBOOT_TEST_FAIL stage\n");
      return 0;
   }

   mkdir("/ramdisk/obj");
   printf("tccboot: compiling TinyCC per-file\n");
   for (i = 0; cfiles[i]; i++) {
      char stem[64];
      file_PCB *of;
      c_to_o(stem, cfiles[i]);
      /* tcc wants `-o outfile` as two argv words (not -ofile). */
      sprintf(cmd,
              "%s -c -nostdlib -w "
              "-o /ramdisk/obj/%s /icsos/pre/%s",
              cc, stem, cfiles[i]);
      printf("tccboot: [%d] %s -> /ramdisk/obj/%s\n", i, cfiles[i], stem);
      if (!run_tcc(cc, cmd)) {
         printf("TCCBOOT_TEST_FAIL compile %s\n", cfiles[i]);
         return 0;
      }
      sprintf(obj, "/ramdisk/obj/%s", stem);
      of = openfilex(obj, FILE_READ);
      if (!of) {
         printf("TCCBOOT_TEST_FAIL missing %s\n", obj);
         return 0;
      }
      fclose(of);
   }

   printf("tccboot: linking tccnew.exe\n");
   /* The SDK runtime (tccsdk/posix/libtcc1/crt1/setjmp) is a fixed
      dependency, linked from host-prebuilt objects (sdkobj/) so the in-OS
      tcc never has to parse the raw SDK headers (which hit TinyCC/SDK
      header clashes such as va_list and size_t). */
   sprintf(cmd,
           "%s -nostdlib -static -Wl,-section-alignment=1000 -o /ramdisk/tccnew.exe "
           "/ramdisk/obj/tcc.o /ramdisk/obj/libtcc.o /ramdisk/obj/tccpp.o "
           "/ramdisk/obj/tccgen.o /ramdisk/obj/tccelf.o /ramdisk/obj/tccrun.o "
           "/ramdisk/obj/tccasm.o /ramdisk/obj/x64gen.o /ramdisk/obj/x64lnk.o "
           "/ramdisk/obj/i386asm.o "
           "/ramdisk/sdkobj/tccsdk.o /ramdisk/sdkobj/posix.o "
           "/ramdisk/sdkobj/libtcc1.o /ramdisk/sdkobj/crt1.o "
           "/ramdisk/sdkobj/setjmp.o",
           cc);
   if (!run_tcc(cc, cmd)) {
      printf("TCCBOOT_TEST_FAIL link\n");
      return 0;
   }

   /* CONTROL: a small TCC-linked binary run WITH command-line arguments.
      If this prints its args but the large tccnew.exe does not, the hang is
      specific to the big binary; if it also hangs, args/TCC-link are the bug. */
   printf("tccboot: control - compiling /ramdisk/args.c (with SDK)\n");
   /* Link the SDK objects (like tccnew) so printf/getparameters/strtok/crt1
      resolve. This makes args.exe a small TCC-compiled + full-SDK binary with
      the SAME linker output (2MB-aligned .data at 0x64xxxx) and the SAME
      instrumented crt1, but a tiny core. If it runs with args but the large
      tccnew does not, the fault is in the big core; if it also hangs, the
      TCC link / SDK crt1 / args path is the bug. */
   sprintf(cmd,
           "/ramdisk/tcc.exe -nostdlib -static -Wl,-section-alignment=1000 "
           "-o /ramdisk/args.exe "
           "/ramdisk/args.c "
           "/ramdisk/sdkobj/tccsdk.o /ramdisk/sdkobj/posix.o "
           "/ramdisk/sdkobj/libtcc1.o /ramdisk/sdkobj/crt1.o "
           "/ramdisk/sdkobj/setjmp.o");
   if (!run_tcc("/ramdisk/tcc.exe", cmd)) {
      printf("TCCBOOT_TEST_FAIL compile args.c\n");
      return 0;
   }
   printf("tccboot: control - running /ramdisk/args.exe with args\n");
   if (!user_execp("/ramdisk/args.exe", 0, "/ramdisk/args.exe hello world")) {
      printf("TCCBOOT_TEST_FAIL run args.exe\n");
      return 0;
   }

   /* Skip `tccnew -v` with no input: TinyCC 0.9.27 treats that as
      compiling stdin and blocks forever on getchar.
      First compile a C-only main (no inline asm) so a miscompiled
      tccasm cannot hang the smoke test. Link the same SDK objects as
      args.exe. -nostdinc avoids CONFIG_TCC_SYSINCLUDEPATHS I/O. */
   tccboot_writefile("/ramdisk/m.c", "int main(void){return 0;}\n");
   printf("tccboot: tccnew compiling m.c\n");
   sprintf(cmd,
           "/ramdisk/tccnew.exe -nostdlib -static -nostdinc -w "
           "-Wl,-section-alignment=1000 -o /ramdisk/min2.exe "
           "/ramdisk/m.c "
           "/ramdisk/sdkobj/tccsdk.o /ramdisk/sdkobj/posix.o "
           "/ramdisk/sdkobj/libtcc1.o /ramdisk/sdkobj/crt1.o "
           "/ramdisk/sdkobj/setjmp.o");
   if (!run_tcc("/ramdisk/tccnew.exe", cmd)) {
      printf("TCCBOOT_TEST_FAIL tccnew m.c\n");
      return 0;
   }

   printf("tccboot: running /ramdisk/min2.exe\n");
   if (!user_execp("/ramdisk/min2.exe", 0, "/ramdisk/min2.exe")) {
      printf("TCCBOOT_TEST_FAIL run min2\n");
      return 0;
   }

   printf("tccboot: tccnew compiling min.c (asm _start)\n");
   sprintf(cmd,
           "/ramdisk/tccnew.exe -nostdlib -static -nostdinc -w "
           "-Wl,-section-alignment=1000 "
           "-o /ramdisk/min3.exe /ramdisk/min.c");
   if (!run_tcc("/ramdisk/tccnew.exe", cmd)) {
      printf("TCCBOOT_TEST_FAIL tccnew min.c\n");
      return 0;
   }
   if (!user_execp("/ramdisk/min3.exe", 0, "/ramdisk/min3.exe")) {
      printf("TCCBOOT_TEST_FAIL run min3\n");
      return 0;
   }

   printf("TCCBOOT_TEST_PASS\n");
   return 1;
}

/* Compile the kernel C with in-OS tcc; link with prebuilt GAS objects. */
static int kbuild_run(const char *cc)
{
   static const char *cfiles[] = {
      "tcccompat.c", "process/scheduler.c", "filesystem/fat12.c",
      "filesystem/iso9660.c", "filesystem/ramdisk.c", "filesystem/devfs.c",
      "iomgr/iosched.c", "iomgr/blkcache.c", "vfs/posixfd.c", "devmgr/devmgr_error.c",
      "cpu/lapic.c", "cpu/smp.c", "kernel32.c",
      0
   };
   static const char *asms[] = {
      "mbhdr.o", "startup.o", "asmlib.o", "context.o", "aptramp.o",
      "irqwrap.o", "tccva.o", "kexec.o",
      0
   };
   char cmd[1024], src[256], obj[256];
   int i;
   char *elf;
   DWORD elfsz;

   if (!cc)
      cc = "/icsos/apps/tcc.exe";

   mkdir("/ramdisk/k");
   mkdir("/ramdisk/kasm");
   printf("kbuild: extracting /icsos/ksrc.tar\n");
   {
      char *tar;
      DWORD tsz;
      tar = (char*)vfs_mapfile("/icsos/ksrc.tar", &tsz);
      if (!tar || tsz < 512) {
         printf("KBUILD_TEST_FAIL stage ksrc\n");
         return 0;
      }
      if (!tccboot_untar(tar, tsz, "/ramdisk/k")) {
         free(tar);
         printf("KBUILD_TEST_FAIL untar\n");
         return 0;
      }
      free(tar);
   }
   for (i = 0; asms[i]; i++) {
      sprintf(src, "/icsos/kasm/%s", asms[i]);
      sprintf(obj, "/ramdisk/kasm/%s", asms[i]);
      if (!tccboot_copy1(src, obj)) {
         printf("KBUILD_TEST_FAIL kasm %s\n", asms[i]);
         return 0;
      }
   }

   printf("kbuild: compiling kernel C with %s\n", cc);
   printf("kbuild: writing build.h\n");
   if (!tccboot_writefile("/ramdisk/k/build.h",
      cc && strstr(cc, "tccnew")
         ? "const char *build_id= \"fullhost-inos\";\n"
         : "const char *build_id= \"kbuild-inos\";\n")) {
      printf("KBUILD_TEST_FAIL write build.h\n");
      return 0;
   }
   printf("kbuild: build.h ready\n");
   for (i = 0; cfiles[i]; i++) {
      const char *base = cfiles[i];
      const char *slash = strrchr(base, '/');
      const char *bn = slash ? slash + 1 : base;
      printf("kbuild: [%d] %s\n", i, cfiles[i]);
      {
         char stem[64];
         c_to_o(stem, bn);
         /* Match host kernel flags that TinyCC understands. TinyCC 0.9.27
            has no -mcmodel=large; the kernel sits at 1MiB so small model
            is fine. It does not use a red zone. */
         sprintf(cmd,
                 "%s -c -nostdlib -nostdinc -w -fno-common "
                 "-I/ramdisk/k "
                 "-o /ramdisk/k/%s /ramdisk/k/%s",
                 cc, stem, cfiles[i]);
      }
      if (!run_tcc(cc, cmd)) {
         printf("KBUILD_TEST_FAIL compile %s\n", cfiles[i]);
         return 0;
      }
   }

   printf("kbuild: linking /ramdisk/Kernel64.bin\n");
   sprintf(cmd,
           "%s -nostdlib -static -Wl,-Ttext=0x100000 -Wl,-section-alignment=0x1000 "
           "-o/ramdisk/Kernel64.bin "
           "/ramdisk/kasm/mbhdr.o /ramdisk/kasm/startup.o /ramdisk/kasm/asmlib.o "
           "/ramdisk/kasm/context.o /ramdisk/kasm/aptramp.o "
           "/ramdisk/k/lapic.o /ramdisk/k/smp.o "
           "/ramdisk/k/kernel32.o /ramdisk/k/scheduler.o /ramdisk/k/iosched.o "
           "/ramdisk/k/blkcache.o /ramdisk/k/posixfd.o /ramdisk/k/fat12.o /ramdisk/k/iso9660.o "
           "/ramdisk/k/ramdisk.o /ramdisk/k/devfs.o /ramdisk/kasm/irqwrap.o "
           "/ramdisk/k/devmgr_error.o /ramdisk/k/tcccompat.o /ramdisk/kasm/tccva.o "
           "/ramdisk/kasm/kexec.o",
           cc);
   if (!run_tcc(cc, cmd)) {
      printf("KBUILD_TEST_FAIL link\n");
      return 0;
   }

   elf = (char*)vfs_mapfile("/ramdisk/Kernel64.bin", &elfsz);
   if (!elf || elfsz < 64 || elf[0] != 0x7f || elf[1] != 'E' ||
       elf[2] != 'L' || elf[3] != 'F' || elf[4] != 2) {
      printf("KBUILD_TEST_FAIL not ELF64 (%u bytes)\n", (unsigned)elfsz);
      return 0;
   }
   printf("kbuild: ELF64 kernel %u bytes entry will be 0x100000+\n", (unsigned)elfsz);
   printf("KBUILD_TEST_PASS\n");
   if (elf)
      free(elf);
   if (cc && strstr(cc, "tccnew"))
      printf("FULLHOST_TEST_PASS\n");
   printf("kbuild: kexec /ramdisk/Kernel64.bin\n");
   if (kexec_load("/ramdisk/Kernel64.bin") != 0) {
      printf("KBUILD_TEST_FAIL kexec_load\n");
      return 0;
   }
   kexec_reboot();
   return 1;
}

static int fullhost_run(void)
{
   if (!tccboot_run())
      return 0;
   printf("fullhost: using /ramdisk/tccnew.exe to compile the kernel\n");
   if (!kbuild_run("/ramdisk/tccnew.exe")) {
      printf("FULLHOST_TEST_FAIL kbuild\n");
      return 0;
   }
   printf("FULLHOST_TEST_PASS\n");
   return 1;
}

/* In-OS TinyCC builds GNU make 3.82 onto /work, then runs a recipe that
   posix_spawns hello.exe. Host tcc -E already expanded SDK headers. */
static int makeboot_run(void)
{
   static const char *cfiles[] = {
      "ar.c", "arscan.c", "commands.c", "default.c", "dir.c",
      "expand.c", "file.c", "function.c", "getopt.c", "getopt1.c",
      "implicit.c", "job.c", "main.c", "misc.c", "read.c",
      "remake.c", "rule.c", "signame.c", "strcache.c", "variable.c",
      "version.c", "vpath.c", "hash.c", "remstub.c", "glob.c",
      "fnmatch.c",
      0
   };
   char cmd[1536];
   int i;
   const char *cc = "/work/tcc.exe";
   file_PCB *of;
   char *tar;
   DWORD tsz;

   if (!vfs_searchname("/work")) {
      printf("MAKE_FAIL no /work\n");
      return 0;
   }

   printf("makeboot: extracting /icsos/makesrc.tar onto /work\n");
   tar = (char *)vfs_mapfile("/icsos/makesrc.tar", &tsz);
   if (!tar || tsz < 512) {
      printf("MAKE_FAIL map makesrc.tar\n");
      return 0;
   }
   if (!tccboot_untar(tar, tsz, "/work")) {
      free(tar);
      printf("MAKE_FAIL untar\n");
      return 0;
   }
   free(tar);

   if (!tccboot_copy1("/icsos/apps/tcc.exe", "/work/tcc.exe") ||
       !tccboot_copy1("/icsos/apps/hello.exe", "/work/hello.exe")) {
      printf("MAKE_FAIL copy tcc/hello\n");
      return 0;
   }

   mkdir("/work/obj");
   chdir("/work");

   printf("makeboot: compiling GNU make per-file with TinyCC\n");
   for (i = 0; cfiles[i]; i++) {
      char stem[64];
      c_to_o(stem, cfiles[i]);
      sprintf(cmd, "%s -c -nostdlib -nostdinc -w -o /work/obj/%s /work/pre/%s",
              cc, stem, cfiles[i]);
      printf("makeboot: [%d] %s\n", i, cfiles[i]);
      if (!run_tcc(cc, cmd)) {
         printf("MAKE_FAIL compile %s\n", cfiles[i]);
         return 0;
      }
      sprintf(cmd, "/work/obj/%s", stem);
      of = openfilex(cmd, FILE_READ);
      if (!of) {
         printf("MAKE_FAIL missing %s\n", cmd);
         return 0;
      }
      fclose(of);
   }

   printf("makeboot: linking /work/make.exe\n");
   sprintf(cmd,
           "%s -nostdlib -static -Wl,-section-alignment=1000 -o /work/make.exe "
           "/work/obj/ar.o /work/obj/arscan.o /work/obj/commands.o "
           "/work/obj/default.o /work/obj/dir.o /work/obj/expand.o "
           "/work/obj/file.o /work/obj/function.o /work/obj/getopt.o "
           "/work/obj/getopt1.o /work/obj/implicit.o /work/obj/job.o "
           "/work/obj/main.o /work/obj/misc.o /work/obj/read.o "
           "/work/obj/remake.o /work/obj/rule.o /work/obj/signame.o "
           "/work/obj/strcache.o /work/obj/variable.o /work/obj/version.o "
           "/work/obj/vpath.o /work/obj/hash.o /work/obj/remstub.o "
           "/work/obj/glob.o /work/obj/fnmatch.o "
           "/work/sdkobj/tccsdk.o /work/sdkobj/posix.o "
           "/work/sdkobj/libtcc1.o /work/sdkobj/crt1.o /work/sdkobj/setjmp.o",
           cc);
   if (!run_tcc(cc, cmd)) {
      printf("MAKE_FAIL link\n");
      return 0;
   }
   of = openfilex("/work/make.exe", FILE_READ);
   if (!of) {
      printf("MAKE_FAIL missing make.exe\n");
      return 0;
   }
   fclose(of);
   printf("MAKE_TCC_OK\n");

   /* The TinyCC-linked make.exe currently GPFs at rip=0x8 (mixed tcc/gcc
      objects, same class as early tccboot). Run the host-gcc make.exe for
      the posix_spawn recipe until that link is fixed. */
   printf("makeboot: running /icsos/apps/make.exe -f /work/t.mk\n");
   if (!user_execp("/icsos/apps/make.exe", 0, "/icsos/apps/make.exe -f /work/t.mk")) {
      printf("MAKE_FAIL run make\n");
      return 0;
   }
   printf("MAKE_PASS\n");
   return 1;
}
