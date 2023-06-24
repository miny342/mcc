#include "mcc.h"

Token *token;
char *user_input;
char *filename;
Function *code;
LVar *locals;
GVar *globals;
String *strs;
int loopcnt = 0;

StrMap *strmap;

char *read_file(char *path) {
    FILE *fp;
    fp = fopen(path, "r");
    if (fp == NULL) {
        error("cannot open %s: %s\n", path, strerror(errno));
    }

    if (fseek(fp, 0, SEEK_END) == -1) {
        error("%s: fseek: %s", path, strerror(errno));
    }
    size_t size = ftell(fp);
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

int main(int argc, char **argv){
    if (argc != 2) {
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }

    locals = calloc(1, sizeof(LVar));
    globals = calloc(1, sizeof(GVar));
    strs = calloc(1, sizeof(String));

    strmap = calloc(1, sizeof(StrMap));

    // user_input = argv[1];
    filename = argv[1];
    user_input = read_file(filename);

    // トークナイズする
    token = tokenize(user_input);
    program();

    gen_global();
    return 0;
}
