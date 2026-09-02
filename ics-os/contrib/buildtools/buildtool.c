/* Minimal POSIX build utilities for native ICS-OS make recipes.
 * Installed under cp.exe, rm.exe and mkdir.exe; dispatch is argv[0].
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

static const char *base(const char *s)
{
   const char *p = strrchr(s, '/');
   return p ? p + 1 : s;
}

static int copy_one(const char *src, const char *dst)
{
   int in, out, n;
   char buf[16384];
   char outpath[512];
   const char *target = dst;
   struct stat st;
   if (stat(dst, &st) == 0 && S_ISDIR(st.st_mode)) {
      if (strlen(dst) + strlen(base(src)) + 2 > sizeof(outpath)) return 1;
      sprintf(outpath, "%s/%s", dst, base(src));
      target = outpath;
   }
   in = open(src, O_RDONLY);
   if (in < 0) {
      fprintf(stderr, "cp: open source failed: %s errno=%d\n", src, errno);
      return 1;
   }
   out = open(target, O_WRONLY | O_CREAT | O_TRUNC, 0666);
   if (out < 0) {
      fprintf(stderr, "cp: open destination failed: %s errno=%d\n", target, errno);
      close(in);
      return 1;
   }
   while ((n = (int)read(in, buf, sizeof(buf))) > 0) {
      int off = 0;
      while (off < n) {
         int w = (int)write(out, buf + off, (size_t)(n - off));
         if (w <= 0) {
            fprintf(stderr, "cp: write failed: %s errno=%d\n", target, errno);
            close(in);
            close(out);
            return 1;
         }
         off += w;
      }
   }
   if (n >= 0 && fsync(out) != 0) {
      fprintf(stderr, "cp: fsync failed: %s errno=%d\n", target, errno);
      close(in);
      close(out);
      return 1;
   }
   close(in);
   close(out);
   if (n < 0)
      fprintf(stderr, "cp: read failed: %s errno=%d\n", src, errno);
   return n < 0;
}

static int cp_main(int argc, char **argv)
{
   int i = 1;
   int verbose = 0;
   while (i < argc && argv[i][0] == '-') {
      if (strcmp(argv[i], "-v") == 0)
         verbose = 1;
      i++;
   }
   if (argc - i != 2) {
      fprintf(stderr, "usage: cp source destination\n");
      return 2;
   }
   if (copy_one(argv[i], argv[i + 1])) {
      fprintf(stderr, "cp: failed: %s -> %s\n", argv[i], argv[i + 1]);
      return 1;
   }
   if (verbose)
      printf("cp: copied %s -> %s\n", argv[i], argv[i + 1]);
   return 0;
}

static int mkdir_one_p(const char *path)
{
   char tmp[512];
   int i, n = (int)strlen(path);
   struct stat st;
   if (n <= 0 || n >= (int)sizeof(tmp)) return 1;
   strcpy(tmp, path);
   for (i = 1; i <= n; i++) {
      if (tmp[i] == '/' || tmp[i] == 0) {
         char save = tmp[i];
         tmp[i] = 0;
          if (tmp[0] && mkdir(tmp, 0777) < 0 &&
             (stat(tmp, &st) < 0 || !S_ISDIR(st.st_mode))) return 1;
         tmp[i] = save;
      }
   }
   return 0;
}

static int mkdir_main(int argc, char **argv)
{
   int i = 1, rc = 0, parents = 0;
   if (i < argc && !strcmp(argv[i], "-p")) { parents = 1; i++; }
   if (i == argc) return 2;
   for (; i < argc; i++) {
      int r = parents ? mkdir_one_p(argv[i]) : (mkdir(argv[i], 0777) < 0);
      if (r) { fprintf(stderr, "mkdir: failed: %s\n", argv[i]); rc = 1; }
   }
   return rc;
}

static int rm_main(int argc, char **argv)
{
   int i = 1, force = 0, rc = 0;
   while (i < argc && argv[i][0] == '-') {
      if (strchr(argv[i], 'f')) force = 1;
      i++;
   }
   for (; i < argc; i++) {
      if (unlink(argv[i]) < 0 && !force) {
         fprintf(stderr, "rm: failed: %s\n", argv[i]);
         rc = 1;
      }
   }
   return rc;
}

int main(int argc, char **argv)
{
   const char *name = base(argv[0]);
   if (!strncmp(name, "cp", 2)) return cp_main(argc, argv);
   if (!strncmp(name, "mkdir", 5)) return mkdir_main(argc, argv);
   if (!strncmp(name, "rm", 2)) return rm_main(argc, argv);
   fprintf(stderr, "buildtool: invoke as cp, mkdir, or rm\n");
   return 2;
}
