/*
 * klog.h — kernel log (dmesg-style) public interface.
 *
 * First version: a timestamped ring buffer that captures kernel printf()
 * output, plus a `dmesg` console command and a live console log-level
 * threshold. User-space console output is NOT captured (only kernel printf),
 * which is the first step toward segregating kernel and user logs.
 *
 * Severity follows syslog convention: 0 is the most severe, KLOG_DEBUG the
 * least. A message is always buffered. It is echoed to the live console only
 * if its level is <= klog_console_max (i.e. at least as severe as the
 * threshold). Default threshold = KLOG_DEBUG, so everything echoes (current
 * behavior is preserved).
 */
#ifndef KLOG_H
#define KLOG_H

#define KLOG_EMERG  0
#define KLOG_ALERT  1
#define KLOG_CRIT   2
#define KLOG_ERR    3
#define KLOG_WARN   4
#define KLOG_NOTICE 5
#define KLOG_INFO   6
#define KLOG_DEBUG  7

/* Initialize the ring (call early in boot, before the console is busy). */
void klog_init(void);

/* Log a formatted message at the given severity (buffered + level-gated echo).
 * Does not recurse through printf(). */
void klog(int level, const char *fmt, ...);

/* Hooks used by console/dexio.c to capture a kernel printf() call as one
 * timestamped record. Begin, then per-character, then end. */
void klog_line_begin(int level);
void klog_line_char(char c);
void klog_line_end(void);

/* True while a kernel printf() is being captured (used by putcEX). */
int  klog_capturing_active(void);
/* Severity of the printf() currently being captured. */
int  klog_current_level(void);

/* Console log-level threshold. */
unsigned char klog_console_max_get(void);
void          klog_console_max_set(unsigned char lvl);

/* Buffer management / inspection. */
void klog_clear(void);
/* Dump the ring oldest-first. level_filter < 0 means "all"; otherwise only
 * records with level <= level_filter are printed. Output goes to the console
 * (not through printf, so it is not re-captured). */
void klog_dump(int level_filter);
unsigned int klog_count(void);

#endif /* KLOG_H */
