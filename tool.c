#include "mcc.h"

void *strmapget(StrMap *map, char *str, int len) {
    StrMap *p = map;
    for(int i = 0; i < len; i++) {
        if (!p->p[str[i]]) {
            return NULL;
        }
        p = p->p[str[i]];
    }
    return p->value;
}

void strmapset(StrMap *map, char *str, int len, void *value) {
    StrMap *p = map;
    for(int i = 0; i < len; i++) {
        if (!p->p[str[i]]) {
            p->p[str[i]] = calloc(1, sizeof(StrMap));
        }
        p = p->p[str[i]];
    }
    p->value = value;
}
