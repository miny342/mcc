#include "libc_alternatives.h"

void *(*(*f(int a))(int))(int);
void (*signal(int, void (*)(int)))(int);
int (*(*testf(void))[10])(void);
int c[12][34];
int (c2[12])[22];

#define ONE 1

int assert_fn(int (*fn)(), char *name, int expect) {
    int res = fn();
    if (res != expect) {
        printf("%s: expected %d, found %d\n", name, expect, res);
        exit(ONE);
    } else {
        printf("ok %s\n", name);
    }
}

#define assert(name, expr, expect) \
int name ## _assert () expr \
int name () {\
    assert_fn(name ## _assert, #name, (expect));\
}
