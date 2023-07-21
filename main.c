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

char *use_qword_reg[REG_NUM] = {"err",
    // "rax", "rdi", "rsi", "rdx", "rcx", "r8", "r9",
    "r10", "r11", "rbp", "rbx", "r12", "r13", "r14", "r15"
};
char *use_dword_reg[REG_NUM] = {"errd",
    // "eax", "edi", "esi", "edx", "ecx", "r8d", "r9d",
    "r10d", "r11d", "ebp", "ebx", "r12d", "r13d", "r14d", "r15d"
};
char *use_word_reg[REG_NUM] = {"errw",
    // "ax", "di", "si", "dx", "cx", "r8w", "r9w",
    "r10w", "r11w", "bp", "bx", "r12w", "r13w", "r14w", "r15w"
};
char *use_byte_reg[REG_NUM] = {"errb",
    // "al", "dil", "sil", "dl", "cl", "r8b", "r9b",
    "r10b", "r11b", "bpl", "bl", "r12b", "r13b", "r14b", "r15b"
};

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

    Vec *functions = gen_global_ir();

    // gen_global();
    gen_function(functions);
    return 0;
}
