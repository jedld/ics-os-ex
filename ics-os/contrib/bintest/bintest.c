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
#include <errno.h>

/* Run tool (path + argv) and wait for it. Returns 0 if spawn+wait succeed. */
static int run_tool(const char *path, char *const argv[])
{
   pid_t pid;
   int st = 0;
   if (posix_spawn(&pid, path, 0, 0, argv, 0) != 0)
      return -1;
   if (waitpid(pid, &st, 0) != pid)
      return -1;
   return 0;
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

int main(void)
{
   /* Input source lives on the read-only CD root; all outputs go to /ramdisk. */
   char *asv[]  = { "/icsos/apps/as.exe",  "--64", "/icsos/mini.s",
                    "-o", "/ramdisk/mini.o", 0 };
   char *arv[]  = { "/icsos/apps/ar.exe",  "/ramdisk/test.a",
                    "/ramdisk/mini.o", 0 };
   char *ldv[]  = { "/icsos/apps/ld.exe",  "/ramdisk/mini.o",
                    "-o", "/ramdisk/mini.exe", 0 };
   char *mini[] = { "/ramdisk/mini.exe", 0 };

   printf("bintest: as --64 mini.s -> /ramdisk/mini.o\n");
   if (run_tool("/icsos/apps/as.exe", asv) != 0 ||
       check_file("/ramdisk/mini.o", "\x7fELF") < 0) {
      printf("AS_FAIL\n");
      return 1;
   }
   printf("AS_PASS\n");

   printf("bintest: ar /ramdisk/test.a /ramdisk/mini.o\n");
   if (run_tool("/icsos/apps/ar.exe", arv) != 0 ||
       check_file("/ramdisk/test.a", "!<arch>") < 0) {
      printf("AR_FAIL\n");
      return 1;
   }
   printf("AR_PASS\n");

   printf("bintest: ld mini.o -> /ramdisk/mini.exe (default script)\n");
   if (run_tool("/icsos/apps/ld.exe", ldv) != 0 ||
       check_file("/ramdisk/mini.exe", "\x7fELF") < 0) {
      printf("LD_FAIL\n");
      return 1;
   }
   printf("LD_PASS\n");

   printf("bintest: exec /ramdisk/mini.exe (linked by ld)\n");
   if (run_tool("/ramdisk/mini.exe", mini) != 0) {
      printf("LD_EXEC_FAIL\n");
      return 1;
   }
   if (check_file("/ramdisk/mini.out", "BINTOOLS_MINI_OK") < 0) {
      printf("LD_EXEC_FAIL\n");
      return 1;
   }
   printf("LD_EXEC_PASS\n");

   printf("BINTOOLS_PASS\n");
   return 0;
}
