/*
 * Hand-written config.h for GCC 4.7.4 (C only) on ICS-OS (x86-64 ELF).
 *
 * Replaces the configure-generated config.h. The compiler (cc1 + the gcc
 * driver) is built with host gcc against the ICS-OS SDK (sdk/include is the
 * libc), then the resulting executables run inside ICS-OS. Same pattern as
 * contrib/binutils.
 *
 * Target: native x86-64 ELF, i386 backend (GCC uses the i386 backend for both
 * 32- and 64-bit x86), single-threaded, C only, no NLS, no plugins.
 *
 * Integer + float constant arithmetic uses GMP + MPFR (era-matched: GMP 4.x /
 * MPFR 3.0.1). Those are built against the SDK separately and linked in.
 */
#ifndef ICSOS_GCC_CONFIG_H
#define ICSOS_GCC_CONFIG_H

#define ICSOS 1
#define HAVE_CONFIG_H 1

/* ---- identity ---- */
#define PACKAGE "gcc"
#define PACKAGE_NAME "GNU C compiler"
#define PACKAGE_VERSION "4.7.4"
#define PACKAGE_STRING "GNU C compiler 4.7.4"
#define PACKAGE_BUGREPORT ""
#define PACKAGE_TARNAME "gcc"
#define VERSION "4.7.4"
#define BASEVER "4.7.4"
#define BUGURL ""
#define DATESTAMP "20260829"
#define PROGRAM_NAME "gcc"

/* ---- target / host identity (i386 x86-64 ELF) ---- */
#define TARGET_SYSTEM "elf"
#define TARGET_MACHINE "i386"
#define TARGET_OS "icsos"
/* Backend default tune: enum constant from config/i386/i386.h. Must be an
   integer (used as a cpu_names[] index), never a string literal. */
#define TARGET_CPU_DEFAULT TARGET_CPU_DEFAULT_generic

/* No LTO plugin in this build (C-only, no plugins). */
#define HAVE_LTO_PLUGIN 0

/* Executable prefix for cpp built-in paths (freestanding: none). */
#define STANDARD_EXEC_PREFIX ""

/* Decimal-float / fixed-point target capabilities (x86_64-linux: BID dfp, no fixed-point). */
#define ENABLE_DECIMAL_FLOAT 1
#define ENABLE_DECIMAL_BID_FORMAT 1
#define ENABLE_FIXED_POINT 0

/* ---- standard C library (provided by the ICS-OS SDK) ---- */
#define STDC_HEADERS 1
#define HAVE_ANSI_COMPILER 1
#define HAVE_LONG_LONG 1
#define HAVE_ALLOCA 1
#define HAVE_VA_COPY 1
#define HAVE_MEMCPY 1
#define HAVE_MEMMOVE 1
#define HAVE_GETOPT 1
#define HAVE_GETOPT_LONG 1
#define HAVE_GETENV 1
#define HAVE_STRTOLL 1
#define HAVE_STRTOULL 1
#define HAVE_STRTOD 1
#define HAVE_STRTOL 1
#define HAVE_STRTODL 1
#define HAVE_STRERROR 1
#define HAVE_STRDUP 1
#define HAVE_STRCASECMP 1
#define HAVE_STRNCASECMP 1
#define HAVE_QSORT 1
#define HAVE_BSEARCH 1
#define HAVE_SIGNAL 1
#define HAVE_GETPAGESIZE 1
#define HAVE_SYSCONF 1
#define HAVE_PATHCONF 1
#define HAVE_GETRLIMIT 1
#define HAVE_SETRLIMIT 1
#define HAVE_UNISTD_H 1
#define HAVE_CLOCK_T 1
/* SDK <time.h> declares clock_t/clock() but not CLOCKS_PER_SEC. */
#ifndef CLOCKS_PER_SEC
#define CLOCKS_PER_SEC 1000000
#endif
#define HAVE_CHDIR 1
#define HAVE_GETCWD 1
#define HAVE_MKDIR 1
#define HAVE_RMDIR 1
#define HAVE_UNLINK 1
#define HAVE_RENAME 1
#define HAVE_STAT 1
#define HAVE_FSTAT 1
#define HAVE_DIRENT_H 1
#define HAVE_LSEEK 1
#define HAVE_READ 1
#define HAVE_WRITE 1
#define HAVE_OPEN 1
#define HAVE_CLOSE 1
#define HAVE_MMAP 1
#define HAVE_MMAP_ANON 1
#define HAVE_MUNMAP 1
#define HAVE_MPROTECT 1
#define HAVE_FSYNC 1
#define HAVE_FTRUNCATE 1
#define HAVE_ACCESS 1
#define HAVE_CHMOD 1
#define HAVE_PIPE 1
#define HAVE_DUP2 1
#define HAVE_ISATTY 1
#define HAVE_GETPID 1
#define HAVE_GETPPID 1
#define HAVE_GETUID 1
#define HAVE_GETEUID 1
#define HAVE_TEMPNAM 1
#define HAVE_TMPNAM 1
#define HAVE_MKSTEMP 1
#define HAVE_MKTEMP 1
#define HAVE_WAITPID 1
#define HAVE_GETTIMEOFDAY 1
#define HAVE_CLOCK 1

/* ---- standard C headers present in the SDK ---- */
#define HAVE_STDDEF_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STDIO_H 1
#define HAVE_STRING_H 1
#define HAVE_STRINGS_H 1
#define HAVE_ERRNO_H 1
#define HAVE_CTYPE_H 1
#define HAVE_TIME_H 1
#define HAVE_FLOAT_H 1
#define HAVE_LIMITS_H 1
#define HAVE_SETJMP_H 1
#define HAVE_ASSERT_H 1
#define HAVE_FCNTL_H 1
#define HAVE_SIGNAL_H 1
#define HAVE_STDBOOL_H 1
#define HAVE_STDINT_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_MATH_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_WAIT_H 1
#define HAVE_SYS_MMAN_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_SYS_RESOURCE_H 1
#define HAVE_SYS_PARAM_H 1
#define HAVE_UNISTD_H 1
#define HAVE_WCHAR_H 1
#define HAVE_STDARG_H 1
#define HAVE_PWD_H 1

/* Intentionally NOT defined (absent from the SDK / unsupported):
 *   HAVE_DLFCN_H (no dlopen -> no plugins), HAVE_MALLOC_H, HAVE_SYS_TIMES_H,
 *   HAVE_KILL, HAVE_FORK, HAVE_VFORK, HAVE_SYS_UTSNAME_H.
 * The sources gate these with #ifdef, so leaving them undefined is correct. */
#define HAVE_MALLOC_H 0

/* ---- host wide-int (x86-64 host: `long` is 64-bit) ----
 * hwint.h (pulled in via gcc/system.h) normally derives HOST_WIDE_INT from
 * HOST_BITS_PER_LONG, but libcpp ships its own system.h that does not include
 * hwint.h and is target-independent. Define it here (guarded) so every
 * component sees it; hwint.h/the target may still override for cc1. */
#ifndef HOST_WIDE_INT
#define HOST_WIDE_INT long
#endif
#ifndef HOST_BITS_PER_WIDE_INT
#define HOST_BITS_PER_WIDE_INT 64
#endif

/* ---- word size / endianness (x86-64) ---- */
#define WORDS_LITTLE_ENDIAN 1
#define HIGHMEM 1
#define SIZEOF_INT 4
#define SIZEOF_LONG 8
#define SIZEOF_LONG_LONG 8
#define SIZEOF_SHORT 2
#define SIZEOF_POINTER 8
#define SIZEOF_SIZE_T 8
#define SIZEOF_PTRDIFF_T 8
#define SIZEOF_WCHAR_T 4
#define SIZEOF_TIME_T 8
#define SIZEOF_OFF_T 8
#define SIZEOF_JMP_BUF 152
#define SIZEOF_WIDE_INT 8
#define SIZEOF_HWFPU_CONTROL_WORDS 0

/* ---- GMP / MPFR (constant arithmetic) ---- */
#define HAVE_GMP 1
#define GMP_VERSION 4
#define HAVE_MPFR 1
#define MPFR_VERSION 3001

/* ---- single-threaded, C only, no NLS ----
 * ENABLE_NLS is intentionally left UNDEFINED: the sources gate it with
 * #ifdef (definition, not value), so defining it 0 would still pull in
 * <libintl.h> (gettext), which the SDK does not provide. */
#define TARGET_USES_64BIT_ICH 0

/* ---- install prefixes inside the OS (nominal; the driver is -B driven) ---- */
#define PREFIX "/work"
#define exec_prefix "/work"
#define bindir "/work/bin"
#define libdir "/work/lib"
#define includedir "/work/include"
#define TOOLDIR "/work"
#define STANDARD_STARTFILE_PREFIX_1 ""
#define STANDARD_STARTFILE_PREFIX_2 ""

/* ---- paths the driver reports (in-OS layout) ---- */
#define INSTALL_TMPDIR "/work/tmp"
#define STANDARD_EXECUTABLE_PREFIX ""
#define STANDARD_STARTFILE_PREFIX ""

#endif /* ICSOS_GCC_CONFIG_H */
