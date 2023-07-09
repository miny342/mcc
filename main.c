#include "mcc.h"

Token *token;
char *user_input;
char *filename;
GVar *code;
LVar *locals;
int max_offset;
String *strs;
int labelcnt = 2;
int continue_label = 0;
int break_label = 0;

int pushcnt = 0;

StrMap *strmap;
StrMap *structmap;
StrMap *enummap;
StrMap *enumkeymap;
StrMap *typenamemap;
StrMap *macromap;

int main(int argc, char **argv){
    if (argc != 2) {
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }

    locals = calloc(1, sizeof(LVar));
    strs = calloc(1, sizeof(String));

    strmap = calloc(1, sizeof(StrMap));
    structmap = calloc(1, sizeof(StrMap));
    enummap = calloc(1, sizeof(StrMap));
    enumkeymap = calloc(1, sizeof(StrMap));
    typenamemap = calloc(1, sizeof(StrMap));
    macromap = calloc(1, sizeof(StrMap));

    // user_input = argv[1];
    filename = argv[1];
    user_input = read_file(filename);

    // トークナイズする
    token = tokenize(user_input, 1);

    // print_token(token);

    token = preprocess();

    // print_token(token);

    // type_test();

    program();

    gen_global();
    return 0;
}
