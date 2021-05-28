#include "mcc.h"

void gen_lval(Node *node) {
    if (node->kind != ND_LVAR)
        error("代入の左辺が変数ではありません");

    printf("  mov rax, rbp\n");
    printf("  sub rax, %d\n", node->offset);
    printf("  push rax\n");
}

// nodeからアセンブリを吐く
void gen(Node *node) {
    int loopval;
    if(node == NULL) return;

    switch (node->kind) {
        case ND_RETURN:
            gen(node->lhs);
            printf("  pop rax\n");
            printf("  mov rsp, rbp\n");
            printf("  pop rbp\n");
            printf("  ret\n");
            return;
        case ND_NUM:
            printf("  push %d\n", node->val);
            return;
        case ND_LVAR:
            gen_lval(node);
            printf("  pop rax\n");
            printf("  mov rax, [rax]\n");
            printf("  push rax\n");
            return;
        case ND_ASSIGN:
            gen_lval(node->lhs);
            gen(node->rhs);

            printf("  pop rdi\n");
            printf("  pop rax\n");
            printf("  mov [rax], rdi\n");
            printf("  push rdi\n");
            return;
        case ND_WHILE:
            loopval = loopcnt;
            loopcnt += 1;
            printf(".Lbegin%d:\n", loopval);
            gen(node->lhs);
            printf("  pop rax\n");
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
            printf("  pop rax\n");
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
            printf("  pop rax\n");
            printf("  cmp rax, 0\n");
            printf("  je .Lend%d\n", loopval);
            gen(node->rhs->rhs->rhs);
            gen(node->rhs->rhs->lhs);
            printf("  jmp .Lbegin%d\n", loopval);
            printf(".Lend%d:\n", loopval);
            return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("  pop rdi\n");
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
    }

    printf("  push rax\n");
}
