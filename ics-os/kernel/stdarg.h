/*
 * Compiler-neutral stdarg for gcc (host kernel) and TinyCC (in-OS kbuild).
 * TinyCC 0.9.27 x86_64 does not implement __builtin_va_start; it emits
 * calls to __va_start/__va_arg (see tccva.c).
 */
#ifndef ICSOS_STDARG_H
#define ICSOS_STDARG_H

#ifdef __TINYC__

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

void __va_start(__va_list_struct *ap, void *fp);
void *__va_arg(__va_list_struct *ap, int arg_type, int size, int align);

#define va_start(ap, last) __va_start(ap, __builtin_frame_address(0))
#define va_arg(ap, type) \
    (*(type *)(__va_arg((ap), __builtin_va_arg_types(type), \
                        sizeof(type), __alignof__(type))))
#define va_end(ap) ((void)0)
#define va_copy(dest, src) (*(dest) = *(src))

#else

typedef __builtin_va_list va_list;
#define va_start(v, l) __builtin_va_start(v, l)
#define va_end(v)      __builtin_va_end(v)
#define va_arg(v, l)   __builtin_va_arg(v, l)
#define va_copy(d, s)  __builtin_va_copy(d, s)

#endif

#endif /* ICSOS_STDARG_H */
