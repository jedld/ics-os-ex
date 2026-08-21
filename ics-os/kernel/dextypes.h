/*
 * dextypes.h — OS version and common types
 */
#ifndef DEXTYPES_H
#define DEXTYPES_H

#include "types.h"

#define DEX32_OSVER 0x00000001
#ifdef __x86_64__
#define DEX32_VERSION_STRING "1.01-x86_64"
#else
#define DEX32_VERSION_STRING "1.01"
#endif

/* Legacy 32-bit recursive map window (unused on x86_64 identity map). */
#define SYS_PAGEDIR_VIR  0xFFC00000
#define SYS_PAGEDIR2_VIR 0xFFC01000
#define SYS_PAGEDIR3_VIR 0xFFC02000
#define SYS_KERPDIR_VIR  0xFFC03000
#define SYS_PAGEDIR4_VIR 0xFFC04000

typedef unsigned short int WORD;
typedef unsigned char BYTE;
typedef unsigned int DWORD;

extern char *dex32_versionstring;

#endif
