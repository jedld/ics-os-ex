#ifndef _STDARG_H
#define _STDARG_H

#if defined(__TINYC__) && defined(__x86_64__)
/*
 * TinyCC 0.9.27 x86_64: struct va_list + libtcc1 __va_start/__va_arg.
 * gcc's __builtin_va_list is not a type in this tcc binary.
 */
typedef struct {
    unsigned int gp_offset;
    unsigned int fp_offset;
    union {
        unsigned int overflow_offset;
        char *overflow_arg_area;
    };
    char *reg_save_area;
} __va_list_struct;

typedef __va_list_struct va_list[1];

/* TinyCC 0.9.27 treats __va_start/__va_arg as builtins; a prototype here
 * is a redefinition (tccboot hit this compiling SDK headers in-OS). */

#define va_start(ap, last) __va_start(ap, __builtin_frame_address(0))
#define va_arg(ap, type)                                                \
    (*(type *)(__va_arg(ap, __builtin_va_arg_types(type), sizeof(type), __alignof__(type))))
#define va_copy(dest, src) (*(dest) = *(src))
#define va_end(ap)

#elif defined(__GNUC__)
typedef __builtin_va_list va_list;
#define va_start(v,l) __builtin_va_start(v,l)
#define va_end(v)     __builtin_va_end(v)
#define va_arg(v,l)   __builtin_va_arg(v,l)
#define va_copy(d,s)  __builtin_va_copy(d,s)

#else
typedef char *va_list;
#define va_start(ap,last) ap = ((char *)&(last)) + ((sizeof(last)+3)&~3)
#define va_arg(ap,type) (ap += (sizeof(type)+3)&~3, *(type *)(ap - ((sizeof(type)+3)&~3)))
#define va_copy(dest, src) ((dest) = (src))
#define va_end(ap)
#endif

typedef va_list __gnuc_va_list;
#define _VA_LIST_DEFINED

#endif
