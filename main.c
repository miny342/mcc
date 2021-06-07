#include "mcc.h"

Token *token;
char *user_input;
Global *code;
LVar *locals;
int loopcnt = 0;

int main(int argc, char **argv){
    if (argc != 2) {
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }

    locals = calloc(1, sizeof(LVar));

    user_input = argv[1];

    // トークナイズする
    token = tokenize(user_input);
    program();

    // アセンブリの前半部分を出力
    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");

    gen_global();
    return 0;
}
