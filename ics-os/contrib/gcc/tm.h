/*
 * tm.h for the ICS-OS GCC build (i386 / x86-64).
 *
 * Build-dir stand-in for the configure-generated target-machine header. For
 * the i386 backend the machine-specific macros live in config/i386/i386.h,
 * which the middle end pulls in via the target object set. libcpp (the first
 * milestone) is target-independent and does not include tm.h, so an empty
 * placeholder is enough to satisfy the include path; the i386 target details
 * are added when cc1 (the middle end + config/i386) is built.
 */
#ifndef ICSOS_GCC_TM_H
#define ICSOS_GCC_TM_H
#endif
