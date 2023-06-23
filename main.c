#include "mcc.h"

Token *token;
char *user_input;
Function *code;
LVar *locals;
LVar *globals;
String *strs;
int loopcnt = 0;

int main(int argc, char **argv){
    if (argc != 2) {
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }

    locals = calloc(1, sizeof(LVar));
    globals = calloc(1, sizeof(LVar));
    strs = calloc(1, sizeof(String));

    user_input = argv[1];

    // トークナイズする
    token = tokenize(user_input);
    program();

    gen_global();
    return 0;
}
