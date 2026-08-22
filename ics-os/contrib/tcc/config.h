#ifndef TCC_CONFIG_H
#define TCC_CONFIG_H

#define TCC_VERSION "0.9.27"
#define CONFIG_TCCDIR "/icsos/tcc1"
#define CONFIG_TCC_SYSINCLUDEPATHS "{B}/include:/icsos/include"
#define CONFIG_TCC_LIBPATHS "{B}"
#define CONFIG_TCC_CRTPREFIX "{B}"
#define CONFIG_TCC_ELFINTERP "/lib64/ld-linux-x86-64.so.2"
#define CONFIG_TCC_STATIC 1
#define CONFIG_TCC_ASM 1
/* Disable -run backtrace / bounds checker (needs signals, mmap extras). */
#define CONFIG_TCCBOOT 1
#define GCC_MAJOR 4
#define GCC_MINOR 8
#define CC_GCC 1

#endif
