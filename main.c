#include "mcc.h"

Token *token;
char *user_input;
Node *code[100];
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
    printf("main:\n");

    // prologue
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    if(locals->offset % 16 != 0)
        printf("  sub rsp, %d\n", locals->offset + 8);
    else
        printf("  sub rsp, %d\n", locals->offset);

    // code gen
    for (int i = 0; code[i]; i++) {
        gen(code[i]);

        // 式の評価結果としてスタックに一つの値が残っているので、スタック溢れを避けるためにポップする
        printf("  pop rax\n");
    }

    // epilogue
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
    return 0;
}
