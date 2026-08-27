/*
 * posix_spawn + waitpid of hello.exe, then a write/read on /work.
 */
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <spawn.h>
#include <sys/wait.h>
#include <errno.h>

static int fail(const char *tag, const char *msg)
{
   printf("spawntest: FAIL %s errno=%d\n", msg, errno);
   printf("%s\n", tag);
   return 1;
}

int main(void)
{
   pid_t pid;
   int st = 0;
   char *argv[] = { "/icsos/apps/hello.exe", 0 };
   int fd, n;
   char buf[32];
   const char *msg = "spawn-ok";

   printf("spawntest: posix_spawn hello.exe\n");
   if (posix_spawn(&pid, argv[0], 0, 0, argv, 0) != 0)
      return fail("SPAWN_FAIL", "posix_spawn");
   if (waitpid(pid, &st, 0) != pid)
      return fail("SPAWN_FAIL", "waitpid");
   printf("SPAWN_PASS\n");

   printf("spawntest: /work disk\n");
   fd = open("/work/spawn.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);
   if (fd < 0)
      return fail("WORK_DISK_FAIL", "open /work/spawn.txt");
   n = (int)write(fd, msg, 8);
   if (n != 8)
      return fail("WORK_DISK_FAIL", "write");
   if (fsync(fd) != 0)
      return fail("WORK_DISK_FAIL", "fsync");
   if (lseek(fd, 0, SEEK_SET) != 0)
      return fail("WORK_DISK_FAIL", "lseek");
   memset(buf, 0, sizeof(buf));
   if (read(fd, buf, 8) != 8 || memcmp(buf, msg, 8) != 0)
      return fail("WORK_DISK_FAIL", "readback");
   close(fd);
   printf("WORK_DISK_PASS\n");
   return 0;
}
