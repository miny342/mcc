#ifdef __GNUC__

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#else

#define NULL 0
#define size_t void * // 事実上の64bit整数...
#define long void *

#define FILE void

extern FILE *stderr;

int fprintf(FILE *stream, char *fmt, ...);
int printf(char *fmt, ...);

#define SEEK_END 2
#define SEEK_SET 0

FILE *fopen(char *filename, char *modes);
int fseek(FILE *stream, long off, long whence);
long ftell(FILE *stream);
size_t fread(void *ptr, size_t size, size_t n, FILE *stream);
int fclose(FILE *stream);

char *strerror(int errnum);

int *__errno_location(void);
#define errno (*__errno_location())


void exit(int status);

void *calloc(size_t len, size_t size);
void *realloc(void *ptr, size_t size);
void *malloc(size_t size);

#define bool char

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
