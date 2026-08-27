#ifndef _DIRENT_H
#define _DIRENT_H

#include <sys/types.h>

struct dirent {
   ino_t d_ino;
   char d_name[256];
};

typedef struct {
   char *packed;
   int off;
   struct dirent de;
} DIR;

DIR *opendir(const char *path);
struct dirent *readdir(DIR *dir);
int closedir(DIR *dir);

#endif
