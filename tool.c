#include "mcc.h"

// topのマップのvalueはローカルへのマップ
void *strmapget(StrMap *map, unsigned char *str, int len) {
    if (!(map && len)) return NULL;
    StrMap *p = map;
    for(int i = 0; i < len; i++) {
        if (!p->p[str[i]]) {
            return strmapget(map->value, str, len);
        }
        p = p->p[str[i]];
    }
    if (p->value) {
        return p->value;
    } else {
        return strmapget(map->value, str, len);
    }
}

int strmapset(StrMap *map, unsigned char *str, int len, void *value) {
    if (!(map && len)) return 0;
    StrMap *p = map;
    for(int i = 0; i < len; i++) {
        if (!p->p[str[i]]) {
            p->p[str[i]] = calloc(1, sizeof(StrMap));
        }
        p = p->p[str[i]];
    }
    p->value = value;
    return 1;  // 成功時1
}

void grow(Vec *v) {
    v->cap *= 2;
    v->data = realloc(v->data, v->cap * (sizeof(void*)));
}

void *at(Vec *v, int i) {
    if (i < 0 || v->len < i) {
        error("index out of range");
    }
    return v->data[i];
}

void push(Vec *v, void *d) {
    if (v->cap == v->len) grow(v);
    v->data[v->len] = d;
    v->len++;
}

Vec *vec_new() {
    Vec *v = malloc(sizeof(Vec));
    v->data = malloc(sizeof(void*) * 4);
    v->cap = 4;
    v->len = 0;
    return v;
}

char *read_file(char *path) {
    FILE *fp;
    fp = fopen(path, "r");
    if (fp == NULL) {
        error("cannot open %s: %s\n", path, strerror(errno));
    }

    if (fseek(fp, 0, SEEK_END) == -1) {
        error("%s: fseek: %s", path, strerror(errno));
    }
    // size_t size = ftell(fp);
    int size = ftell(fp);
    if (fseek(fp, 0, SEEK_SET) == -1) {
        error("%s: fseek: %s", path, strerror(errno));
    }

    char *buf = malloc(sizeof(char) * (size + 2));
    fread(buf, size, 1, fp);

    if (size == 0 || buf[size - 1] != '\n') {
        buf[size++] = '\n';
    }
    buf[size] = '\0';
    fclose(fp);
    return buf;
}

