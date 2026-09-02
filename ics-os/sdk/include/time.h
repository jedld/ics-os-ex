#ifndef _TIME_H
#define _TIME_H

#include <stddef.h>

typedef long time_t;
typedef long clock_t;

struct tm {
    int tm_sec, tm_min, tm_hour;
    int tm_mday, tm_mon, tm_year;
    int tm_wday, tm_yday, tm_isdst;
};

time_t time(time_t *t);
clock_t clock(void);
struct tm *localtime(const time_t *t);
char *ctime(const time_t *t);
char *asctime(const struct tm *);
size_t strftime(char *s, size_t max, const char *fmt, const struct tm *tm);

/* Monotonic millisecond clock from the kernel (getprecisetime, syscall
   0x96). Used by full-screen apps (vim) for timeout/syntax timing. */
#define CLOCK_MONOTONIC 1
#define CLOCK_REALTIME  0

struct timespec {
    long tv_sec;
    long tv_nsec;
};

int clock_gettime(int clockid, struct timespec *tp);

#endif
