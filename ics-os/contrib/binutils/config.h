/*
 * Hand-written config.h for GNU binutils 2.23 on ICS-OS (x86-64 ELF).
 *
 * This replaces the configure-generated config.h. The target is a native
 * x86_64 ELF host whose "libc" is the ICS-OS SDK (tccsdk.c / posix.c).
 * We build as/ld/ar with host gcc against sdk/include, then run them in-OS.
 *
 * Keep this minimal: no NLS, no plugins, single target (elf64-x86-64).
 */
#ifndef ICSOS_BINUTILS_CONFIG_H
#define ICSOS_BINUTILS_CONFIG_H

#define ICSOS 1
#define HAVE_CONFIG_H 1

/* Host / target identity */
#define HOST_SYSTEM "icsos"
#define TARGET_SYSTEM "elf64-x86-64"
#define BFD_DEFAULT_TARGET "elf64-x86-64"
#define TARGET_ARCH "i386"
#define TARGET_MACHINE ""

/* The one target we build: x86-64 ELF */
#define TARGET_DEFAULT elf64_littleswap

/* GNU debuglink search directory (dwarf2.c). A path string literal. */
#define DEBUGDIR "/debug"

/* No plugins (needs dlopen), no NLS/gettext, no demangling C++ */
#define ENABLE_PLUGINS 0
#define DISABLE_NLS 1
#define HAVE_W32_API 0

/* Compiler / C library features the SDK provides. */
#define STDC_HEADERS 1
#define HAVE_ANSI_COMPILER 1
#define HAVE_LONG_LONG 1
#define HAVE_ALLOCA 1
#define HAVE_VA_COPY 1
#define HAVE_MEMCPY 1
#define HAVE_MEMMOVE 1
#define HAVE_BZERO 0
#define HAVE_BCMP 0
#define HAVE_BCOPY 0
#define HAVE_GETOPT 1
#define HAVE_GETOPT_LONG 1
#define HAVE_GETENV 1
#define HAVE_GETTIMEOFDAY 1
#define HAVE_STRTOLL 1
#define HAVE_STRTOULL 1
#define HAVE_STRTOD 1
#define HAVE_STRTOL 1
#define HAVE_STRTODL 1
#define HAVE_STRERROR 1
#define HAVE_STRDUP 1
#define HAVE_STRCASECMP 1
#define HAVE_STRNCASECMP 1
#define HAVE_MEMMEM 0
#define HAVE_QSORT 1
#define HAVE_BSEARCH 1
#define HAVE_SIGNAL 1
#define HAVE_SIGACTION 1
#define HAVE_SYS_SIGLIST 0
#define HAVE_SETJMP_H 1
#define HAVE_ASSERT_H 1
#define HAVE_FCNTL_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_SYS_TYPES_H 1
/* Standard C + BSD headers the SDK provides (libiberty/libbfd gate includes
   on these). */
#define HAVE_STDDEF_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STDIO_H 1
#define HAVE_STRING_H 1
#define HAVE_ERRNO_H 1
#define HAVE_CTYPE_H 1
#define HAVE_TIME_H 1
#define HAVE_FLOAT_H 1
#define HAVE_MALLINFO_H 0
#define HAVE_MALLOC_H 0
#define HAVE_SYS_PARAM_H 1
#define HAVE_SYS_RESOURCE_H 1
#define HAVE_GETPAGESIZE 1
#define HAVE_SYSCONF 1
#define HAVE_PATHCONF 1
#define HAVE_FCHDIR 0
#define HAVE_GETRLIMIT 1
#define HAVE_SETRLIMIT 1
#define HAVE_UNISTD_H 1
#define HAVE_STDINT_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_LIMITS_H 1
#define HAVE_MALLINFO 0
#define HAVE_MADVISE 0
#define HAVE_MLOCK 0
#define HAVE_FTRUNCATE 1
#define HAVE_FSYNC 1
#define HAVE_CHDIR 1
#define HAVE_GETCWD 1
#define HAVE_REALPATH 1
#define HAVE_MKSTEMP 1
#define HAVE_MKTEMP 1
#define HAVE_ACCESS 1
#define HAVE_CHMOD 1
#define HAVE_FORK 0
#define HAVE_VFORK 0
#define HAVE_PIPE 1
#define HAVE_DUP2 1
#define HAVE_ISATTY 1
#define HAVE_SLEEP 1
#define HAVE_ALARM 0
#define HAVE_KILL 0
#define HAVE_GETUID 1
#define HAVE_GETEUID 1
#define HAVE_GETGID 1
#define HAVE_GETEGID 1
#define HAVE_GETPPID 1
#define HAVE_GETPID 1
#define HAVE_GETEXECNAME 0
#define HAVE_TEMPNAM 1
#define HAVE_TMPNAM 1
#define HAVE_TEMPNAM_R 1
#define HAVE_MKDIR 1
#define HAVE_RMDIR 1
#define HAVE_UNLINK 1
#define HAVE_RENAME 1
#define HAVE_CHOWN 0
#define HAVE_FCHDIR 0
#define HAVE_READLINK 0
#define HAVE_SYMLINK 0
#define HAVE_LSTAT 0
#define HAVE_STAT 1
#define HAVE_FSTAT 1
#define HAVE_DIRENT_H 1
#define HAVE_OPENDIR 1
#define HAVE_READDIR 1
#define HAVE_CLODIR 1
#define HAVE_LSEEK 1
#define HAVE_READ 1
#define HAVE_WRITE 1
#define HAVE_OPEN 1
#define HAVE_CLOSE 1
#define HAVE_CREAT 1
#define HAVE_TRUNCATE 1
#define HAVE_LCHOWN 0
#define HAVE_CHROOT 0
#define HAVE_PWD_H 1
#define HAVE_GRP_H 0
#define HAVE_SYS_MMAN_H 1
#define HAVE_MMAP 1
#define HAVE_MUNMAP 1
#define HAVE_MPROTECT 1
#define HAVE_MLOCKALL 0
#define HAVE_SHM_GET 0
#define HAVE_MACH_O_DYLD_H 0
#define HAVE_DLFCN_H 0
#define HAVE_DLOPEN 0
#define HAVE_DLERROR 0
#define HAVE_DLADDR 0
#define HAVE_SYS_PARAM_H 0
#define HAVE_SYS_RESOURCE_H 0
#define HAVE_SYS_UTSNAME_H 0
#define HAVE_UTSNAME 0
#define HAVE_UNISTD_H 1
#define HAVE_POSIX_SPAWN_H 1
#define HAVE_WAITPID 1
#define HAVE_SYS_WAIT_H 1
#define HAVE_SYS_UIO_H 1
#define HAVE_PREAD 1
#define HAVE_PWRITE 1
#define HAVE_PREADV 1
#define HAVE_PWRITEV 1
#define HAVE_FDATASYNC 0
#define HAVE_SYNC_FILE_RANGE 0
#define HAVE_CLOCK 1
#define HAVE_CLOCK_GETTIME 0
#define HAVE_TIMESPEC 0
#define HAVE_SIGSET_T 1
#define HAVE_STRUCT_SIGACTION 1
#define HAVE_SA_SIGINFO 1
#define HAVE_SIGINFO_T 1
#define HAVE_STRUCT_TIMEVAL 1
#define HAVE_STRUCT_TIMESPEC 1
#define HAVE_TM_GMTOFF 0
#define HAVE_STRUCT_TM 1
#define HAVE_LOCALTIME_R 0
#define HAVE_GMTIME_R 0
#define HAVE_GETTIMEZONE 0
#define HAVE_TZNAME 0
#define HAVE_STRFTIME 0
#define HAVE_MBSTOWCS 0
#define HAVE_WCTOMB 0
#define HAVE_MBSTATE_T 0
#define HAVE_WPRINTF 0
#define HAVE_MMAP64 0
#define HAVE_FTRUNCATE64 0
#define HAVE_OFF64_T 0
#define HAVE_STAT64 0
#define HAVE_FSTAT64 0
#define HAVE_LSTAT64 0
#define HAVE_LARGEFILE 0
#define WORDS_LITTLE_ENDIAN 1
#define HIGHMEM 1
#define SIZEOF_UNSIGNED_INT 4
#define SIZEOF_UNSIGNED_LONG 8
#define SIZEOF_INT 4
#define SIZEOF_LONG 8
#define SIZEOF_LONG_LONG 8
#define SIZEOF_SIZE_T 8
#define SIZEOF_PTRDIFF_T 8
#define SIZEOF_TIME_T 8
#define SIZEOF_PID_T 4
#define SIZEOF_OFF_T 8
#define SIZEOF_RLIMIT_T 8
#define SIZEOF_WCHAR_T 4
#define SIZEOF_MODE_T 4
#define SIZEOF_UID_T 4
#define SIZEOF_GID_T 4
#define SIZEOF_DEV_T 8
#define SIZEOF_INO_T 8
#define SIZEOF_NLINK_T 8
#define SIZEOF_CLOCK_T 8
#define SIZEOF_SYSDEVT 0
#define SIZEOF_INTMAX_T 8
#define SIZEOF_UINTMAX_T 8
#define SIZEOF_OFF_T 8
#define SIZEOF_SSIZE_T 8
#define SIZEOF_TIME_T 8
#define SIZEOF_PTRDIFF_T 8
#define SIZEOF_WCHAR_T 4

/* NOTE: mode_t, pid_t, off_t, time_t, clock_t, uid_t, gid_t, dev_t, ino_t,
   nlink_t, size_t, ssize_t, ptrdiff_t, intptr_t, uintptr_t and the fixed-width
   int types are defined by the ICS-OS SDK headers (sys/types.h, stddef.h,
   stdint.h). Do NOT redefine them here. intmax_t/uintmax_t (C99) are added to
   the SDK's stdint.h. */

#define INO_T_MAX ((ino_t)-1)
#define NLINK_T_MAX ((nlink_t)-1)
#define UID_T_MAX ((uid_t)-1)
#define GID_T_MAX ((gid_t)-1)
#define INO64_T_MAX ((ino64_t)-1)
#define NLINK64_T_MAX ((nlink64_t)-1)
#define OFF64_T_MAX ((off64_t)-1)
#define RSIZE_MAX ((ssize_t)-1)
#define UINTMAX_T_MAX ((uintmax_t)-1)
#define ULONG_MAX_64 ((unsigned long)-1)
#define OFF_T_MAX ((off_t)-1)
#define ULONG_LONG_MAX_64 ((unsigned long long)-1)

/* No C++ demangling, no threads, no shared libs */
#define HAVE_CPP 0
#define HAVE_LIBIBERTY 1
#define TARGET_LINKS_DYNEXE 0
#define TARGET_LINKS_RELOC 1
#define TARGET_USES_64BIT_ICH 0
#define BFD64 1
#define BFD64_BYTEORDER BFD_BYTEORDER_LITTLE_ENDIAN
#define BFD_DEFAULT_TARGET_BYTEORDER BFD_BYTEORDER_LITTLE_ENDIAN

/* Paths inside the OS */
#define INCLUDEDIR "/work/include"
#define LIBDIR "/work/lib"
#define BINDIR "/work/bin"
#define LOCALEDIR ""
#define PREFIX "/work"
#define exec_prefix "/work"
#define libdir "/work/lib"
#define includedir "/work/include"
#define bindir "/work/bin"

#define PACKAGE "binutils"
#define PACKAGE_NAME "GNU binutils"
#define PACKAGE_STRING "GNU binutils 2.23"
#define PACKAGE_VERSION "2.23"
#define PACKAGE_BUGREPORT ""
#define PACKAGE_TARNAME "binutils"
#define VERSION "2.23"
#define BFD_VERSION "2.23-icsos"
#define BFD_TARGETS "elf64-x86-64"

#define alloca __builtin_alloca

#endif
