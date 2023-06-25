#include "mcc.h"
#define min(x, y) ((x) <= (y) ? (x) : (y))

void gen_global() {
    // アセンブリの前半部分を出力
    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");

    for(; code; code = code->next) {
        printf("%.*s:\n", code->len, code->name);

        // prologue
        printf("  push rbp\n");
        printf("  mov rbp, rsp\n");

        for(int i = 0; i < code->arglen; i++) {
            switch (i) {
                case 0:
                    printf("  push rdi\n");
                    break;
                case 1:
                    printf("  push rsi\n");
                    break;
                case 2:
                    printf("  push rdx\n");
                    break;
                case 3:
                    printf("  push rcx\n");
                    break;
                case 4:
                    printf("  push r8\n");
                    break;
                case 5:
                    printf("  push r9\n");
                    break;
            }
        }

        int sub = (code->locals->offset - min(code->arglen, 6) * 8) + (16 - code->locals->offset % 16) % 16;

        if (sub > 0) {
            printf("  sub rsp, %d\n", sub);
        }

        gen(code->node);

        // epilogue
        printf("  xor eax, eax\n");
        printf("  leave\n");
        printf("  ret\n");
    }

    printf(".data\n");
    for(; globals->next; globals = globals->next) {
        printf("%.*s:\n", globals->len, globals->name);
        int size = globals->offset - gen_gvar(globals, globals->node);
        if (size > 0) {
            printf("  .zero %d\n", size);
        }
    }

    printf(".section .rodata\n");
    for(; strs->next; strs = strs->next) {
        printf(".LC%d:\n", strs->offset);
        printf("  .string %.*s\n", strs->tok->len, strs->tok->str);
    }
}

int gen_gvar(GVar *gvar, Node *node) {
    int i = 0;
    int size;
    if(!node) return 0;

    switch (node->kind) {
        case ND_BLOCK:
            if(node->lhs) {
                size = sizeof_parse(node->lhs->type);
                if (size == 8) {
                    printf("  .quad");
                } else if (size == 4) {
                    printf("  .long");
                } else {
                    printf("  .byte");
                }
                i += gen_gvar(gvar, node->lhs);
                printf("\n");
                i += gen_gvar(gvar, node->rhs);
                i += size;
            }
            return i;
        case ND_ADD:
            i += gen_gvar(gvar, node->lhs);
            printf(" +");
            i += gen_gvar(gvar, node->rhs);
            return i;
        case ND_SUB:
            i += gen_gvar(gvar, node->lhs);
            printf(" -");
            i += gen_gvar(gvar, node->rhs);
            return i;
        case ND_MUL:
            i += gen_gvar(gvar, node->lhs);
            printf(" *");
            i += gen_gvar(gvar, node->rhs);
            return i;
        case ND_ADDR:
            printf(" %.*s", node->lhs->gvar->len, node->lhs->gvar->name);
            return i;
        case ND_NUM:
            printf(" %d", node->val);
            return i;
        case ND_STR:
            printf(" .LC%d", node->s->offset);
            return i;
        default:
            error("gen global error %d", node->kind);
    }
}

void gen_lval(Node *node) {
    if(node->kind == ND_DEREF) {
        gen(node->lhs);
        return;
    }
    if (node->kind == ND_GLOVAL_LVAR) {
        printf("  lea rax, %.*s[rip]\n", node->gvar->len, node->gvar->name);
        return;
    }
    if (node->kind != ND_LVAR) {
        error("代入の左辺が変数ではありません");
    }
    if(node->lvar->offset > 0)
        printf("  lea rax, [rbp-%d]\n", node->lvar->offset);
    else
        printf("  lea rax, [rbp+%d]\n", -node->lvar->offset);
}

void gen_callstack(Node *node, int num) {
    if(node->lhs) {
        gen_callstack(node->rhs, num + 1);
        gen(node->lhs);
        printf("  push rax\n");
    } else if (num > 6 && num % 2 == 1)
        printf("  sub rsp, 8\n");
}

int gen_callregister(Node *node, int num) {
    if (!(node->rhs)) {
        if(num > 6) {
            if (num % 2 == 1) {
                return num - 5;
            }
            return num - 6;
        }
        return 0;
    }
    switch (num){
        case 0:
            printf("  pop rdi\n");
            break;
        case 1:
            printf("  pop rsi\n");
            break;
        case 2:
            printf("  pop rdx\n");
            break;
        case 3:
            printf("  pop rcx\n");
            break;
        case 4:
            printf("  pop r8\n");
            break;
        case 5:
            printf("  pop r9\n");
            break;
    }
    return gen_callregister(node->rhs, num + 1);
}

void gen_call(Node *node) {
    if (node->kind != ND_CALL)
        error("this is not call function");

    int stack;

    gen_callstack(node, 0);
    stack = gen_callregister(node, 0);
    printf("  xor rax, rax\n");
    printf("  call %.*s\n", node->len, node->name);
    if (stack > 0) {
        printf("  add rsp, %d\n", stack * 8);
    }
}

// nodeからアセンブリを吐く
// raxに結果を残す
void gen(Node *node) {
    int loopval;
    int size;
    if(node == NULL) return;

    switch (node->kind) {
        case ND_RETURN:
            gen(node->lhs);
            printf("  leave\n");
            printf("  ret\n");
            return;
        case ND_NUM:
            printf("  mov rax, %d\n", node->val);
            return;
        case ND_LVAR:
            gen_lval(node);
            size = sizeof_parse(node->lvar->type);
            if (size == 4) {
                printf("  movsx rax, dword ptr [rax]\n");
            } else if (size == 1) {
                printf("  movzx rax, byte ptr [rax]\n");
            } else {
                printf("  mov rax, qword ptr [rax]\n");
            }
            return;
        case ND_ASSIGN:
            gen_lval(node->lhs);
            printf("  push rax\n");
            gen(node->rhs);

            printf("  pop rdi\n");
            size = sizeof_parse(node->lhs->type);
            if (size == 4) {
                printf("  mov dword ptr [rdi], eax\n");
            } else if (size == 1) {
                printf("  mov byte ptr [rdi], al\n");
            } else {
                printf("  mov qword ptr [rdi], rax\n");
            }
            return;
        case ND_WHILE:
            loopval = loopcnt;
            loopcnt += 1;
            printf(".Lbegin%d:\n", loopval);
            gen(node->lhs);
            printf("  cmp rax, 0\n");
            printf("  je .Lend%d\n", loopval);
            gen(node->rhs);
            printf("  jmp .Lbegin%d\n", loopval);
            printf(".Lend%d:\n", loopval);
            return;
        case ND_IF:
            loopval = loopcnt;
            loopcnt += 1;
            gen(node->lhs);
            printf("  cmp rax, 0\n");
            if(node->rhs->kind == ND_ELSE) {
                printf("  je .Lelse%d\n", loopval);
                gen(node->rhs->lhs);
                printf("  jmp .Lend%d\n", loopval);
                printf(".Lelse%d:\n", loopval);
                gen(node->rhs->rhs);
                printf(".Lend%d:\n", loopval);
            } else {
                printf("  je .Lend%d\n", loopval);
                gen(node->rhs);
                printf(".Lend%d:\n", loopval);
            }
            return;
        case ND_FOR:
            loopval = loopcnt;
            loopcnt += 1;
            gen(node->lhs);
            printf(".Lbegin%d:\n", loopval);
            gen(node->rhs->lhs);
            printf("  cmp rax, 0\n");
            printf("  je .Lend%d\n", loopval);
            gen(node->rhs->rhs->rhs);
            gen(node->rhs->rhs->lhs);
            printf("  jmp .Lbegin%d\n", loopval);
            printf(".Lend%d:\n", loopval);
            return;
        case ND_BLOCK:
            gen(node->rhs);
            gen(node->lhs);
            return;
        case ND_CALL:
            gen_call(node);
            return;
        case ND_ADDR:
            gen_lval(node->lhs);
            return;
        case ND_DEREF:
            gen(node->lhs);
            size = sizeof_parse(node->lhs->type->ptr_to);
            if (size == 4) {
                printf("  movsx rax, dword ptr [rax]\n");
            } else if (size == 1) {
                printf("  movzx rax, byte ptr [rax]\n");
            } else {
                printf("  mov rax, qword ptr [rax]\n");
            }
            return;
        case ND_GLOVAL_LVAR:
            gen_lval(node);
            size = sizeof_parse(node->gvar->type);
            if (size == 4) {
                printf("  movsx rax, dword ptr [rax]\n");
            } else if (size == 1) {
                printf("  movzx rax, byte ptr [rax]\n");
            } else {
                printf("  mov rax, qword ptr [rax]\n");
            }
            return;
        case ND_STR:
            printf("  lea rax, .LC%d[rip]\n", node->s->offset);
            return;
        case ND_AND:
            loopval = loopcnt;
            loopcnt += 1;
            gen(node->lhs);
            printf("  test rax, rax\n");  // ZF = (rax != 0)
            printf("  je .Land%d\n", loopval);  // je: ZFが1ならjump <=> raxが0ならjump
            gen(node->rhs);
            printf("  test rax, rax\n");
            printf(".Land%d:\n", loopval);
            printf("  setne al\n");  // !ZFを格納
            printf("  movzb rax, al\n");
            return;
        case ND_OR:
            loopval = loopcnt;
            loopcnt += 1;
            gen(node->lhs);
            printf("  test rax, rax\n");
            printf("  jne .Lor%d\n", loopval); // jne: ZFが0ならjump <=> raxが0でないならjump
            gen(node->rhs);
            printf("  test rax, rax\n");
            printf(".Lor%d:\n", loopval);
            printf("  setne al\n");
            printf("  movzb rax, al\n");
            return;
        case ND_DECLARATION:
            gen(node->lhs);
            return;
    }

    gen(node->lhs);
    printf("  push rax\n");
    gen(node->rhs);

    printf("  mov rdi, rax\n");
    printf("  pop rax\n");
    switch(node->kind) {
        case ND_ADD:
            printf("  add rax, rdi\n");
            break;
        case ND_SUB:
            printf("  sub rax, rdi\n");
            break;
        case ND_MUL:
            printf("  imul rax, rdi\n");
            break;
        case ND_DIV:
            printf("  cqo\n");
            printf("  idiv rdi\n");
            break;
        case ND_EQ:
            printf("  cmp rax, rdi\n");
            printf("  sete al\n");
            printf("  movzb rax, al\n");
            break;
        case ND_NE:
            printf("  cmp rax, rdi\n");
            printf("  setne al\n");
            printf("  movzb rax, al\n");
            break;
        case ND_LE:
            printf("  cmp rax, rdi\n");
            printf("  setle al\n");
            printf("  movzb rax, al\n");
            break;
        case ND_LT:
            printf("  cmp rax, rdi\n");
            printf("  setl al\n");
            printf("  movzb rax, al\n");
            break;
        case ND_REMINDER:
            printf("  cqo\n");
            printf("  idiv rdi\n");
            printf("  mov rax, rdx\n");
            break;
        case ND_LSHIFT:
            printf("  mov rcx, rdi\n");
            printf("  shl rax, cl\n");
            break;
        case ND_RSHIFT:
            printf("  mov rcx, rdi\n");
            printf("  sar rax, cl\n");
            break;
        case ND_BITAND:
            printf("  and rax, rdi\n");
            break;
        case ND_BITOR:
            printf("  or rax, rdi\n");
            break;
        case ND_BITXOR:
            printf("  xor rax, rdi\n");
            break;
    }
}
