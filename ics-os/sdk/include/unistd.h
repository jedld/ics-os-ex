#ifndef _UNISTD_H
#define _UNISTD_H

#include <stddef.h>
#include <sys/types.h>

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#define F_OK 0
#define R_OK 4
#define W_OK 2
#define X_OK 1

int close(int fd);
ssize_t read(int fd, void *buf, size_t n);
ssize_t write(int fd, const void *buf, size_t n);
long lseek(int fd, long off, int whence);
ssize_t pread(int fd, void *buf, size_t n, off_t offset);
ssize_t pwrite(int fd, const void *buf, size_t n, off_t offset);
int fsync(int fd);
int unlink(const char *path);
int rmdir(const char *path);
int mkdir(const char *path, mode_t mode);
char *getcwd(char *buf, size_t size);
int chdir(const char *path);
int isatty(int fd);
int access(const char *path, int mode);
int chmod(const char *path, mode_t mode);
void *sbrk(int inc);
unsigned int sleep(unsigned int seconds);
void _exit(int status);
int getpid(void);
int kill(int pid, int sig);
pid_t vfork(void);
char *getlogin(void);
int getloadavg(double loadavg[], int nelem);
int fork(void);
int execv(const char *path, char *const argv[]);
int execvp(const char *file, char *const argv[]);
int dup2(int oldfd, int newfd);
int pipe(int fd[2]);
int umask(int mask);
int getuid(void);
int geteuid(void);
int getgid(void);
int getegid(void);
int mkstemp(char *template);

extern char **environ;

#endif
