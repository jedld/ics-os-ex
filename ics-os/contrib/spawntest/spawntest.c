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

int main(int argc, char **main_argv)
{
   pid_t pid;
   int st = 0;
   char *argv[] = { "/icsos/apps/hello.exe", 0 };
   int fd, n;
   char buf[32];
   const char *msg = "spawn-ok";

   if (argc == 3 && strcmp(main_argv[1], "fd-child") == 0) {
      fd=main_argv[2][0]-'0';
      if (fd < 3 || fd > 9 || write(fd,"inherit-ok",10) != 10 || fsync(fd) != 0)
         return fail("FD_INHERIT_FAIL", "child inherited write");
      printf("FD_INHERIT_CHILD_PASS\n");
      return 0;
   }

   printf("spawntest: posix_spawn hello.exe\n");
   if (posix_spawn(&pid, argv[0], 0, 0, argv, 0) != 0)
      return fail("SPAWN_FAIL", "posix_spawn");
   if (waitpid(pid, &st, 0) != pid)
      return fail("SPAWN_FAIL", "waitpid");
   printf("SPAWN_PASS\n");

   printf("spawntest: inherited fd survives parent close\n");
   fd=open("/work/inherit.txt",O_RDWR|O_CREAT|O_TRUNC,0666);
   if (fd < 3 || fd > 9)
      return fail("FD_INHERIT_FAIL", "open inherited file");
   {
      char fdarg[2];
      char *child_argv[]={"/icsos/apps/spawn.exe","fd-child",fdarg,0};
      fdarg[0]=(char)('0'+fd);
      fdarg[1]=0;
      if (posix_spawn(&pid,child_argv[0],0,0,child_argv,0) != 0)
         return fail("FD_INHERIT_FAIL", "spawn fd child");
      if (close(fd) != 0)
         return fail("FD_INHERIT_FAIL", "parent close");
      if (waitpid(pid,&st,0) != pid || st != 0)
         return fail("FD_INHERIT_FAIL", "wait fd child");
   }
   fd=open("/work/inherit.txt",O_RDONLY,0);
   if (fd < 0)
      return fail("FD_INHERIT_FAIL", "reopen inherited file");
   memset(buf,0,sizeof(buf));
   if (read(fd,buf,10) != 10 || memcmp(buf,"inherit-ok",10) != 0)
      return fail("FD_INHERIT_FAIL", "verify inherited write");
   close(fd);
   printf("FD_INHERIT_PASS\n");

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
