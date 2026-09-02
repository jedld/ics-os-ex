/* In-OS smoke test for the native tools required by the kernel Makefile. */
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <spawn.h>
#include <sys/wait.h>

extern char **environ;

static int run(char *path, char **argv)
{
   pid_t pid;
   int status = -1;
   if (posix_spawn(&pid, path, 0, 0, argv, environ) != 0) return 0;
   if (waitpid(pid, &status, 0) < 0) return 0;
   return 1;
}

static int is_elf(const char *path)
{
   unsigned char h[5];
   int fd = open(path, O_RDONLY);
   int n;
   if (fd < 0) return 0;
   n = (int)read(fd, h, sizeof(h));
   close(fd);
   return n == 5 && h[0] == 0x7f && h[1] == 'E' && h[2] == 'L' &&
          h[3] == 'F' && h[4] == 2;
}

int main(void)
{
   char *mkdirv[] = { "/icsos/apps/mkdir.exe", "-p", "/ramdisk/build/sub", 0 };
   char *cpv[] = { "/icsos/apps/cp.exe", "/icsos/vmdex", "/ramdisk/build/sub", 0 };
   char *debugv[] = { "/icsos/apps/objcopy.exe", "--only-keep-debug", "/ramdisk/build/sub/vmdex", "/ramdisk/kernel.sym", 0 };
   char *stripv[] = { "/icsos/apps/objcopy.exe", "--strip-debug", "/ramdisk/build/sub/vmdex", 0 };
   char *rmv[] = { "/icsos/apps/rm.exe", "-f", "/ramdisk/kernel.sym", 0 };
   int fd;

   if (!run(mkdirv[0], mkdirv) || !run(cpv[0], cpv) ||
      !is_elf("/ramdisk/build/sub/vmdex")) goto fail;
   printf("BUILDTOOLS_CP_MKDIR_OK\n");
   if (!run(debugv[0], debugv) || !is_elf("/ramdisk/kernel.sym")) goto fail;
   if (!run(stripv[0], stripv) || !is_elf("/ramdisk/build/sub/vmdex")) goto fail;
   printf("BUILDTOOLS_OBJCOPY_OK\n");
   if (!run(rmv[0], rmv)) goto fail;
   fd = open("/ramdisk/kernel.sym", O_RDONLY);
   if (fd >= 0) { close(fd); goto fail; }
   printf("BUILDTOOLS_RM_OK\nBUILDTOOLS_TEST_PASS\n");
   return 0;
fail:
   printf("BUILDTOOLS_TEST_FAIL\n");
   return 1;
}
