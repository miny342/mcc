void *(*(*f(int a))(int))(int);
void (*signal(int, void (*)(int)))(int);
int (*(*testf(void))[10])(void);
int c[12][34];
int (c2[12])[22];

#define ONE 1

#define assert(name, expr, expect) \
int name ## _assert () expr \
int name () {\
    int name ## _i;\
    name ## _i = name ## _assert ();\
    if(name ## _i != (expect)) {\
        printf("%s: expected %d, found %d\n", #name, (expect), name ## _i);\
        exit(ONE);\
    } else printf("ok %s\n", #name);\
}
