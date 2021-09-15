#include <stdio.h>
#include <stdlib.h>

void print(int i) {
    printf("%d\n", i);
}

int *alloc4(int a, int b, int c, int d) {
    int *ret = malloc(sizeof(int) * 8);
    ret[0] = a;
    ret[1] = b;
    ret[2] = c;
    ret[3] = d;
    return ret;
}

void printPTR(int *i) {
    printf("%d\n", i[1]);
}
