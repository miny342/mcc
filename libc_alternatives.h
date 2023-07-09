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
#define unsigned

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

long strtol(char *nptr, char **endptr, int base);

char *strerror(int errnum);

size_t strlen(char *s);
int memcmp(void *s1, void *s2, size_t n);
int strncmp(char *s1, char *s2, size_t n);
char *strstr(char *haystack, char *needle);
char *strchr(char *s, int c);
void *memcpy(void *dest, void *src, size_t n);

int *__errno_location(void);
#define errno (*__errno_location())

// ctypesから
#define _ISbit(bit)	((bit) < 8 ? ((1 << (bit)) << 8) : ((1 << (bit)) >> 8))
enum
{
  _ISupper = _ISbit (0),	/* UPPERCASE.  */
  _ISlower = _ISbit (1),	/* lowercase.  */
  _ISalpha = _ISbit (2),	/* Alphabetic.  */
  _ISdigit = _ISbit (3),	/* Numeric.  */
  _ISxdigit = _ISbit (4),	/* Hexadecimal numeric.  */
  _ISspace = _ISbit (5),	/* Whitespace.  */
  _ISprint = _ISbit (6),	/* Printing.  */
  _ISgraph = _ISbit (7),	/* Graphical.  */
  _ISblank = _ISbit (8),	/* Blank (usually SPC and TAB).  */
  _IScntrl = _ISbit (9),	/* Control character.  */
  _ISpunct = _ISbit (10),	/* Punctuation.  */
  _ISalnum = _ISbit (11)	/* Alphanumeric.  */
};

// ほんとはshort int**
int **__ctype_b_loc(void);
#define __isctype(c, type) ((*__ctype_b_loc())[(int)(c)] & (int) type)
#define isalnum(c)	__isctype((c), _ISalnum)
#define isalpha(c)	__isctype((c), _ISalpha)
#define iscntrl(c)	__isctype((c), _IScntrl)
#define isdigit(c)	__isctype((c), _ISdigit)
#define islower(c)	__isctype((c), _ISlower)
#define isgraph(c)	__isctype((c), _ISgraph)
#define isprint(c)	__isctype((c), _ISprint)
#define ispunct(c)	__isctype((c), _ISpunct)
#define isspace(c)	__isctype((c), _ISspace)
#define isupper(c) __isctype((c), _ISupper)
#define isxdigit(c) __isctype((c), _ISxdigit)

void exit(int status);

void *calloc(size_t len, size_t size);
void *realloc(void *ptr, size_t size);
void *malloc(size_t size);
void free(void *ptr);

#define bool char
#define true 1
#define false 0

typedef struct {
    int gp_offset;  // 汎用引数レジスタ
    int fp_offset;
    void *overflow_arg_area;
    void *reg_save_area;
} va_list[1];

void __va_start(va_list ap, void *offset_reg);

#define va_start(ap, param) __va_start(ap, &param)

int vfprintf(FILE *stream, char *fmt, va_list ap);

#endif
