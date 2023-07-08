#ifdef __GNUC__

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#else

#define va_start(ap, param) __va_start(ap, &param)

#ifdef va_start

typedef struct {
    int gp_offset;  // 汎用引数レジスタ
    int fp_offset;
    void *overflow_arg_area;
    void *reg_save_area;
} va_list[1];

#endif

#endif
