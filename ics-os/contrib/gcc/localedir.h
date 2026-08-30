/*
 * localedir.h stand-in for the configure-generated header.
 * No NLS on ICS-OS -> empty LOCALEDIR (libcpp init.c only uses it for the
 * default message-catalog search dir, which is unused with ENABLE_NLS off).
 */
#ifndef ICSOS_GCC_LOCALEDIR_H
#define ICSOS_GCC_LOCALEDIR_H
#define LOCALEDIR ""
#endif
