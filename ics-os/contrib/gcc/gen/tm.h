#ifndef GCC_TM_H
#define GCC_TM_H
#ifndef TARGET_128BIT_LONG_DOUBLE
# define TARGET_128BIT_LONG_DOUBLE 0
#endif
#ifndef USE_IX86_FRAME_POINTER
# define USE_IX86_FRAME_POINTER 1
#endif
#ifndef LIBC_GLIBC
# define LIBC_GLIBC 1
#endif
#ifndef LIBC_UCLIBC
# define LIBC_UCLIBC 2
#endif
#ifndef LIBC_BIONIC
# define LIBC_BIONIC 3
#endif
#ifndef DEFAULT_LIBC
# define DEFAULT_LIBC LIBC_GLIBC
#endif
#ifdef IN_GCC
# include "vxworks-dummy.h"
# include "i386/biarch64.h"
# include "i386/i386.h"
# include "i386/unix.h"
# include "i386/att.h"
# include "dbxelf.h"
# include "elfos.h"
# include "gnu-user.h"
# include "glibc-stdint.h"
# include "i386/x86-64.h"
# include "i386/gnu-user64.h"
# include "linux.h"
# include "i386/linux64.h"
#endif
#if defined IN_GCC && !defined GENERATOR_FILE && !defined USED_FOR_TARGET
# include "insn-flags.h"
# include "insn-constants.h"
# include "machmode.h"
# include "tm-preds.h"
# include "flags.h"
#endif
# include "defaults.h"
#endif /* GCC_TM_H */
