#ifndef _STDARG_H
#define _STDARG_H

#if defined(__x86_64__) || (defined(__GNUC__) && !defined(__TINYC__))
typedef __builtin_va_list va_list;
#define va_start(v,l) __builtin_va_start(v,l)
#define va_end(v)     __builtin_va_end(v)
#define va_arg(v,l)   __builtin_va_arg(v,l)
#define va_copy(d,s)  __builtin_va_copy(d,s)
#else
/* TinyCC / i386 stack-based varargs */
typedef char *va_list;
#define va_start(ap,last) ap = ((char *)&(last)) + ((sizeof(last)+3)&~3)
#define va_arg(ap,type) (ap += (sizeof(type)+3)&~3, *(type *)(ap - ((sizeof(type)+3)&~3)))
#define va_copy(dest, src) ((dest) = (src))
#define va_end(ap)
#endif

typedef va_list __gnuc_va_list;
#define _VA_LIST_DEFINED

#endif
