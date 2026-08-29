/*
 * bintest: in-OS functional test of the ICS-OS binutils (as / ar / ld).
 *
 * Runs the host-built tools (int 0x30 ABI, staged in /icsos/apps) from inside
 * the OS via posix_spawn + waitpid and verifies their OUTPUT FILES, because
 * the kernel does not propagate a child's exit code through waitpid
 * (process.c stores waitq_st=0). Every check therefore reads the artifact the
 * child is supposed to produce:
 *
 *   as  --64 /icsos/mini.s -o /ramdisk/mini.o        -> .o is a valid ELF
 *   ar  /ramdisk/test.a /ramdisk/mini.o              -> archive ("!<arch>")
 *   ld  /ramdisk/mini.o -o /ramdisk/mini.exe         -> .exe is a valid ELF
 *   (posix_spawn) /ramdisk/mini.exe                  -> writes
 *                                                       "BINTOOLS_MINI_OK" to
 *                                                       /ramdisk/mini.out
 *
 * mini.s is a self-contained x86-64 program (its own _start, raw int 0x30
 * syscalls, no SDK runtime): it is the object the as->ld->exec chain operates
 * on, so a green run proves the assembler emits a linkable ELF, the linker
 * resolves its _start and writes a loadable ELF, and the kernel ELF64 loader
 * can load and run the result. All intermediates live on /ramdisk (16 MiB
 * RAM disk, mounted at boot) so the test needs no virtio /work disk.
 *
 * ld is invoked WITHOUT -T: it must locate the default script
 * (/icsos/apps/ldscripts/elf_x86_64.x) via find_scripts_dir()'s
 * "next to the binary" fallback. That is exactly the mechanism the later
 * GCC self-host relies on, so it is what we validate here.
 */
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <spawn.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>

/* Run tool (path + argv) and wait for it. Returns 0 if spawn+wait succeed. */
static int run_tool(const char *path, char *const argv[])
{
   pid_t pid = 0;
   int st = 0;
   int r;
   r = posix_spawn(&pid, path, 0, 0, argv, 0);
    if (r != 0) {
       printf("  spawn %s -> errno=%d\n", path, r);
       return -1;
    }
    /* Wait BEFORE printing: the child writes to the same serial console and
       the SDK printf is char-at-a-time, so printing while the child runs
       interleaves the two streams into an unreadable blob. The child's own
       output then appears as clean lines above. */
    if (waitpid(pid, &st, 0) != pid) {
       printf("  waitpid %s failed\n", path);
       return -1;
    }
    printf("  ran %s pid=%d st=0x%x\n", path, (int)pid, (unsigned)st);
    return 0;
}

/* Report a file's size (lseek to end) + first bytes, to localize tool
 * failures. Distinguishes "file absent", "created empty", "created with
 * N bytes of X". */
static void diag_file(const char *path)
{
   int fd, n, i, nz, first_nz = -1;
   long sz;
   unsigned char buf[640];
   fd = open(path, O_RDONLY);
   if (fd < 0) {
      printf("  diag %s: open failed\n", path);
      return;
   }
   sz = lseek(fd, 0, 2);          /* SEEK_END */
   if (sz > 0)
      lseek(fd, 0, 0);            /* back to start */
   n = (int)read(fd, buf, sz < 640 ? sz : 640);
   if (n < 0)
      n = 0;
   close(fd);
   nz = 0;
   for (i = 0; i < n; i++) {
      if (buf[i]) {
         nz++;
         if (first_nz < 0)
            first_nz = i;
      }
   }
   printf("  diag %s: size=%ld nonzero=%d first_nz_at=%d hex=",
          path, (long)sz, nz, first_nz);
   for (i = 0; i < n && i < 64; i++)
      printf("%02x", buf[i]);
   printf(" ...\n");
}

/* Open path, read up to sizeof buf bytes, close. Returns bytes read (<0 fail).
 * If magic is non-NULL, the first strlen(magic) bytes must match it. */
static int check_file(const char *path, const char *magic)
{
   int fd, n;
   char buf[64];
   fd = open(path, O_RDONLY);
   if (fd < 0)
      return -1;
   n = (int)read(fd, buf, sizeof(buf));
   close(fd);
   if (n < 1)
      return -1;
   if (magic && n >= (int)strlen(magic) && memcmp(buf, magic, strlen(magic)) != 0)
      return -1;
   return n;
}

/* On-disk size of path via a fresh O_RDONLY + lseek(SEEK_END). */
static long disk_size(const char *path)
{
  int fd = open(path, O_RDONLY);
  if (fd < 0)
     return -1;
  long sz = lseek(fd, 0, SEEK_END);
  close(fd);
  return sz;
}

int main(void)
{
   /* Probe: SDK snprintf width behavior (ar "File too big" investigation). */
   {
      char sbuf[32];
      int sr = snprintf(sbuf, sizeof(sbuf), "%-10llu", 1032ULL);
      int si;
      for (si = 0; si < 16 && (unsigned char)sbuf[si]; si++) { }
      printf("bintest: snprintf %%-10llu ret=%d len=%d hex=", sr, si);
      {
         int k;
         for (k = 0; k < si; k++) printf("%02x ", sbuf[k]);
         printf("\n");
      }
   }
   /* Input is first copied to /ramdisk (CD reads in a spawned child are under
      suspicion); all tool outputs go to /ramdisk. */
   char *asv[]  = { "/icsos/apps/as.exe",  "--64", "/ramdisk/mini.s",
                    "-o", "/ramdisk/mini.o", 0 };
   /* GNU ar requires an operation; bare "ar archive.o" just prints usage.
      "r" = replace/add members. */
   char *arv[]  = { "/icsos/apps/ar.exe",  "r", "/ramdisk/test.a",
                    "/ramdisk/mini.o", 0 };
   char *ldv[]  = { "/icsos/apps/ld.exe",  "/ramdisk/mini.o",
                    "-o", "/ramdisk/mini.exe", 0 };
   char *mini[] = { "/ramdisk/mini.exe", 0 };

   /* Probe: can a user process read the input from the CD root? */
   {
      int cfd = open("/icsos/mini.s", O_RDONLY);
      if (cfd < 0)
         printf("bintest: PROBE open /icsos/mini.s FAILED (%d)\n", cfd);
      else {
         long csz = lseek(cfd, 0, 2);
         if (csz > 0)
            lseek(cfd, 0, 0);
         {
            unsigned char cb[24];
            int cn = (int)read(cfd, cb, sizeof(cb));
            int ci;
            printf("bintest: PROBE /icsos/mini.s size=%ld read=%d: ",
                   (long)csz, cn);
            for (ci = 0; ci < cn && ci < 24; ci++)
               printf("%02x", cb[ci]);
            printf("\n");
         }
         close(cfd);
      }
   }

   /* Probe the DEX C-library file layer (fopen/fread/fwrite/fclose) - the
      path binutils uses. The POSIX open/read above proved the CD is readable
      via the fd layer; this checks the FILE-handle layer as actually uses. */
   {
      FILE *rf = fopen("/icsos/mini.s", "r");
      if (!rf) {
         printf("bintest: DEXPROBE fopen(/icsos/mini.s,r) FAILED\n");
      } else {
         unsigned char rb[24];
         size_t rn = fread(rb, 1, sizeof(rb), rf);
         int ri;
         printf("bintest: DEXPROBE fopen(/icsos/mini.s,r) fread=%d: ", (int)rn);
         for (ri = 0; ri < (int)rn && ri < 24; ri++)
            printf("%02x", rb[ri]);
         printf("\n");
         fclose(rf);
      }
      /* DEX write + readback to /ramdisk (same layer as -o output). */
      {
         FILE *wf = fopen("/ramdisk/dexprobe.out", "w");
         if (!wf) {
            printf("bintest: DEXPROBE fopen(/ramdisk/dexprobe.out,w) FAILED\n");
         } else {
            const char *msg = "DEXWRITE_OK";
            size_t wn = fwrite(msg, 1, strlen(msg), wf);
            fclose(wf);
            printf("bintest: DEXPROBE fwrite=%d\n", (int)wn);
            FILE *rf2 = fopen("/ramdisk/dexprobe.out", "r");
            if (!rf2) {
               printf("bintest: DEXPROBE reopen read FAILED\n");
            } else {
               char rb2[32];
               size_t rn2 = fread(rb2, 1, sizeof(rb2), rf2);
               int ri2;
               printf("bintest: DEXPROBE readback=%d: ", (int)rn2);
               for (ri2 = 0; ri2 < (int)rn2 && ri2 < 32; ri2++)
                  printf("%02x", (unsigned char)rb2[ri2]);
               printf("\n");
               fclose(rf2);
            }
         }
      }
   }

  /* Copy the CD input to /ramdisk so `as` reads a ramdisk file, isolating
     "can a spawned child read the CD" from "as's own I/O pattern". */
  {
     int sf = open("/icsos/mini.s", O_RDONLY);
     int df = open("/ramdisk/mini.s", O_WRONLY | O_CREAT | O_TRUNC);
     if (sf < 0 || df < 0) {
        printf("bintest: COPY setup failed (src=%d dst=%d)\n", sf, df);
     } else {
        char cbuf[256];
        int n, total = 0;
        while ((n = (int)read(sf, cbuf, sizeof(cbuf))) > 0) {
           int w = 0;
           while (w < n) {
              int mw = write(df, cbuf + w, n - w);
              if (mw <= 0)
                 break;
              w += mw;
           }
           total += w;
        }
        close(sf);
        close(df);
        printf("bintest: COPY /icsos/mini.s -> /ramdisk/mini.s %d bytes\n", total);
     }
  }

  /* Empty-file size probe: does FAT pre-allocate a cluster on create? If a
     brand-new file opened for write (no data) already reports size>0, then
     "512 zeros" from `as` means `as` wrote 0 bytes, not a partial object. */
  {
     FILE *ef = fopen("/ramdisk/empty.w", "w");
     if (ef) {
        long esz = ftell(ef);
        fclose(ef);
        printf("bintest: EMPTYPROBE new-file size after w+close = %ld\n", (long)esz);
     } else {
        printf("bintest: EMPTYPROBE fopen(w) failed\n");
     }
  }

  /* Large DEX write in the PARENT: 2048 bytes forces several 512-byte buffer
     flushes (the path `as` uses for an object file). If this round-trips,
     the DEX write path is fine and the `as` failure is child-specific. */
  {
     static char big[2048];
     int i;
     for (i = 0; i < 2048; i++)
        big[i] = (char)(i & 0xff);
     FILE *lf = fopen("/ramdisk/large.w", "w");
     if (!lf) {
        printf("bintest: BIGPROBE fopen failed\n");
     } else {
        size_t wn = fwrite(big, 1, sizeof(big), lf);
        fclose(lf);
        FILE *lr = fopen("/ramdisk/large.w", "r");
        if (!lr) {
           printf("bintest: BIGPROBE reopen failed\n");
        } else {
           char rb[2048];
           size_t rn = fread(rb, 1, sizeof(rb), lr);
           int ok = (rn == 2048);
           for (i = 0; ok && i < 2048; i++)
              if ((unsigned char)rb[i] != (i & 0xff))
                 ok = 0;
           printf("bintest: BIGPROBE wrote=%d read=%d match=%d\n",
                  (int)wn, (int)rn, ok);
           fclose(lr);
        }
     }
  }

  /* DEX r+ seeked-write regression probe (BFD's bfd_openw pattern). Write
     "0123456789", reopen r+, seek(3)+fwrite("XX") -> "012XX56789". The DEX
     readback and a POSIX cross-read are compared: a DEX read of 0 with a
     correct POSIX read localizes the fault to the DEX file_PCB read state
     after an intervening r+ open/close (the ar/ld gate). */
  {
     FILE *f = fopen("/ramdisk/seek.w", "w");
     if (f) {
        fwrite("0123456789", 1, 10, f);
        fclose(f);
        FILE *g = fopen("/ramdisk/seek.w", "r+");
        if (g) {
           fseek(g, 3, SEEK_SET);
           fwrite("XX", 1, 2, g);
           fclose(g);
           char rb[16];
           size_t rn = 0;
           FILE *h = fopen("/ramdisk/seek.w", "r");
           if (h) {
              fseek(h, 0, SEEK_END);
              long hsize = ftell(h);
              fseek(h, 0, SEEK_SET);
              printf("bintest: SEEKPROBE DEX-handle size(SEEK_END)=%ld\n",
                     hsize);
              rn = fread(rb, 1, sizeof(rb) - 1, h);
              fclose(h);
           }
           if (rn < sizeof(rb) - 1)
              rb[rn] = 0;
           printf("bintest: SEEKPROBE DEX-read=%d: [%s]\n", (int)rn, rb);
           int pf = open("/ramdisk/seek.w", O_RDONLY);
           int pr = 0;
           if (pf >= 0) {
              pr = (int)read(pf, rb, sizeof(rb) - 1);
              close(pf);
           }
           if (pr < 0)
              pr = 0;
           if (pr < (int)sizeof(rb) - 1)
              rb[pr] = 0;
           printf("bintest: SEEKPROBE POSIX-read=%d: [%s] (want [012XX56789])\n",
                  pr, rb);
        }
     }
  }

  /* POSIX O_RDWR probe (the mode BFD's bfd_openw uses for its output): open
     read-write, write, lseek back, write, close, read back. */
  {
     int of = open("/ramdisk/rw.w", O_RDWR | O_CREAT | O_TRUNC);
     if (of < 0) {
        printf("bintest: RWPROBE open(rw) failed\n");
     } else {
        write(of, "0123456789", 10);
        close(of);
        printf("bintest: RWPROBE after w size=%ld\n", disk_size("/ramdisk/rw.w"));
        int of2 = open("/ramdisk/rw.w", O_RDWR);
        if (of2 < 0) {
           printf("bintest: RWPROBE reopen(rw) failed\n");
        } else {
           lseek(of2, 3, SEEK_SET);
           write(of2, "XX", 2);
           close(of2);
           printf("bintest: RWPROBE size-after=%ld\n",
                  disk_size("/ramdisk/rw.w"));
           int rf = open("/ramdisk/rw.w", O_RDONLY);
           char rb[16];
           int rn = 0;
           if (rf >= 0) {
              rn = (int)read(rf, rb, sizeof(rb) - 1);
              close(rf);
           }
           if (rn < 0)
              rn = 0;
           if (rn < (int)sizeof(rb) - 1)
              rb[rn] = 0;
           printf("bintest: RWPROBE read=%d: [%s] (want [012XX56789])\n",
                  rn, rb);
        }
     }
  }

   /* POSIX seeked-write probe (ld's path): lseek+write to an existing file.
      If this works but the DEX one above empties the file, the bug is the DEX
      buffered seek/write path (stale buffer), not the FS write/size logic. */
   {
      int of = open("/ramdisk/pseek.w", O_WRONLY | O_CREAT | O_TRUNC);
      if (of < 0) {
         printf("bintest: PSEEKPROBE open(w) failed\n");
      } else {
         write(of, "0123456789", 10);
         lseek(of, 3, SEEK_SET);
         write(of, "XX", 2);
         close(of);
         int rf = open("/ramdisk/pseek.w", O_RDONLY);
         char rb[16];
         int rn = 0;
         if (rf >= 0) {
            rn = (int)read(rf, rb, sizeof(rb) - 1);
            close(rf);
         }
         if (rn < 0)
            rn = 0;
         if (rn < (int)sizeof(rb) - 1)
            rb[rn] = 0;
         printf("bintest: PSEEKPROBE read=%d: [%s] (want [012XX56789])\n",
                rn, rb);
      }
   }

 printf("bintest: as --64 mini.s -> /ramdisk/mini.o\n");
   if (run_tool("/icsos/apps/as.exe", asv) != 0) {
      diag_file("/ramdisk/mini.o");
      printf("AS_FAIL\n");
      return 1;
   }
   diag_file("/ramdisk/mini.o");
   /* \177ELF = the ELF magic. Octal escape: \x7fE would greedily consume the
      'E' (a hex digit) and produce 0xFE, so never write "\x7fELF" here. */
   if (check_file("/ramdisk/mini.o", "\177ELF") < 0) {
      printf("AS_FAIL\n");
      return 1;
   }
   printf("AS_PASS\n");

   printf("bintest: ar /ramdisk/test.a /ramdisk/mini.o\n");
   if (run_tool("/icsos/apps/ar.exe", arv) != 0) {
      diag_file("/ramdisk/test.a");
      printf("AR_FAIL\n");
      return 1;
   }
   diag_file("/ramdisk/test.a");
   if (check_file("/ramdisk/test.a", "!<arch>") < 0) {
      printf("AR_FAIL\n");
      return 1;
   }
   printf("AR_PASS\n");

   printf("bintest: ld mini.o -> /ramdisk/mini.exe (default script)\n");
    {
      int ldr = run_tool("/icsos/apps/ld.exe", ldv);
       if (ldr != 0) {
         diag_file("/ramdisk/mini.exe");
         printf("LD_FAIL\n");
         return 1;
      }
    }
   diag_file("/ramdisk/mini.exe");
   if (check_file("/ramdisk/mini.exe", "\177ELF") < 0) {
      printf("LD_FAIL\n");
      return 1;
   }
   printf("LD_PASS\n");

   printf("bintest: exec /ramdisk/mini.exe (linked by ld)\n");
   if (run_tool("/ramdisk/mini.exe", mini) != 0) {
      diag_file("/ramdisk/mini.out");
      printf("LD_EXEC_FAIL\n");
      return 1;
   }
   diag_file("/ramdisk/mini.out");
   if (check_file("/ramdisk/mini.out", "BINTOOLS_MINI_OK") < 0) {
      printf("LD_EXEC_FAIL\n");
      return 1;
   }
   printf("LD_EXEC_PASS\n");

   printf("BINTOOLS_PASS\n");
   return 0;
}
