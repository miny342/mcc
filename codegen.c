#include "mcc.h"
#define min(x, y) ((x) <= (y) ? (x) : (y))

char *byte_reg[] = {"dil", "sil", "dl", "cl", "r8b", "r9b"};
char *dword_reg[] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};
char *qword_reg[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

char *ptr_name(int size) {
    switch (size) {
        case 1:
            return "byte ptr";
        case 4:
            return "dword ptr";
        case 8:
            return "qword ptr";
        default:
            error("ptr_name is not implemented %d\n", size);
    }
}

char **regs(int size) {
    switch (size) {
        case 1:
            return byte_reg;
        case 4:
            return dword_reg;
        case 8:
            return qword_reg;
        default:
            error("regs is not implemented %d\n", size);
    }
}

void gen_global() {
    // アセンブリの前半部分を出力
    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");

    GVar *data;
    GVar **tmp = &data;

    for(; code; code = code->next) {
        if (code->type->ty == FUNC && code->node) {
            Type *type = code->type;
            printf("%.*s:\n", code->len, code->name);

            // prologue
            printf("  push rbp\n");
            printf("  mov rbp, rsp\n");

            int sub = (code->offset) + (16 - code->offset % 16) % 16;
            if (sub > 0)
                printf("  sub rsp, %d\n", sub);

            Type *args = type->args;
            int offset = 0;
            for(int i = 0; i < 6 && i < type->arglen; i++) {
                int size = sizeof_parse(args);
                offset = calc_aligned(offset, args);
                printf("  mov %s [rbp-%d], %s\n", ptr_name(size), offset, regs(size)[i]);
                args = args->args;
            }

            gen(code->node);

            // epilogue
            printf("  xor eax, eax\n");
            printf("  leave\n");
            printf("  ret\n");
        } else if (code->type->ty != FUNC) {
            *tmp = code;
            tmp = &code->next;
        }
    }
    *tmp = NULL;

    printf(".data\n");
    for(; data; data = data->next) {
        printf("%.*s:\n", data->len, data->name);
        int size = data->size - gen_gvar(data->node);
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

int gen_gvar(Node *node) {
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
                i += gen_gvar(node->lhs);
                printf("\n");
                i += gen_gvar(node->rhs);
                i += size;
            }
            return i;
        case ND_ADD:
            i += gen_gvar(node->lhs);
            printf(" +");
            i += gen_gvar(node->rhs);
            return i;
        case ND_SUB:
            i += gen_gvar(node->lhs);
            printf(" -");
            i += gen_gvar(node->rhs);
            return i;
        case ND_MUL:
            i += gen_gvar(node->lhs);
            printf(" *");
            i += gen_gvar(node->rhs);
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
    if (node->kind == ND_GVAR) {
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

int gen_callstack(Node *node, int num) {
    int i;
    if(node->lhs) {
        i = gen_callstack(node->rhs, num + 1);
        gen(node->lhs);
        printf("  push rax\n");
        return i;
    } else {
        i = num > 6 ? num - 6 : 0;
        if ((pushcnt + i) & 1) {
            printf("  sub rsp, 8\n");
            i += 1;
        }
        return i;
    }
}

void gen_callregister(Node *node) {
    int num = 0;
    node = node->rhs;
    while(node && num < 6) {
        printf("  pop %s\n", qword_reg[num]);
        num++;
        node = node->rhs;
    }
}

void gen_call(Node *node) {
    if (node->kind != ND_CALL)
        error("this is not call function");

    int stack = gen_callstack(node->rhs, 0);

    if (node->lhs->kind == ND_GVAR) {
        gen_callregister(node->rhs);
        printf("  xor rax, rax\n");
        printf("  call %.*s\n", node->lhs->gvar->len, node->lhs->gvar->name);
    } else {
        gen(node->lhs);
        gen_callregister(node->rhs);
        printf("  mov r10, rax\n");
        printf("  xor rax, rax\n");
        printf("  call r10\n");
    }
    if (stack > 0) {
        printf("  add rsp, %d\n", stack * 8);
        pushcnt -= stack;
    }
}

// nodeからアセンブリを吐く
// raxに結果を残す
void gen(Node *node) {
    int loopval, size, clabel, blabel;
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
            if (node->type->ty == ARRAY) {
                return;
            }
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
            pushcnt += 1;
            gen(node->rhs);

            printf("  pop rdi\n");
            pushcnt -= 1;
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
            loopval = labelcnt;
            labelcnt += 2;
            blabel = break_label;
            clabel = continue_label;
            break_label = loopval + 1;
            continue_label = loopval;
            printf(".L%d:\n", loopval);
            gen(node->lhs);
            printf("  cmp rax, 0\n");
            printf("  je .L%d\n", loopval + 1);
            gen(node->rhs);
            printf("  jmp .L%d\n", loopval);
            printf(".L%d:\n", loopval + 1);
            break_label = blabel;
            continue_label = clabel;
            return;
        case ND_IF:
            loopval = labelcnt;
            if(node->rhs->kind == ND_ELSE) {
                labelcnt += 2;
                gen(node->lhs);
                printf("  cmp rax, 0\n");
                printf("  je .L%d\n", loopval);
                gen(node->rhs->lhs);
                printf("  jmp .L%d\n", loopval + 1);
                printf(".L%d:\n", loopval);
                gen(node->rhs->rhs);
                printf(".L%d:\n", loopval + 1);
            } else {
                labelcnt += 1;
                gen(node->lhs);
                printf("  cmp rax, 0\n");
                printf("  je .L%d\n", loopval);
                gen(node->rhs);
                printf(".L%d:\n", loopval);
            }
            return;
        case ND_FOR:
            loopval = labelcnt;
            labelcnt += 2;
            blabel = break_label;
            clabel = continue_label;
            break_label = loopval + 1;
            continue_label = 1;
            gen(node->lhs);
            printf(".L%d:\n", loopval);
            gen(node->rhs->lhs);
            printf("  cmp rax, 0\n");
            printf("  je .L%d\n", loopval + 1);
            gen(node->rhs->rhs->rhs);
            if (continue_label > 1) {
                printf(".L%d:\n", continue_label);
            }
            gen(node->rhs->rhs->lhs);
            printf("  jmp .L%d\n", loopval);
            printf(".L%d:\n", loopval + 1);
            continue_label = clabel;
            break_label = blabel;
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
            if (node->type->ty == ARRAY) {
                return;
            }
            size = sizeof_parse(node->lhs->type->ptr_to);
            if (size == 4) {
                printf("  movsx rax, dword ptr [rax]\n");
            } else if (size == 1) {
                printf("  movzx rax, byte ptr [rax]\n");
            } else {
                printf("  mov rax, qword ptr [rax]\n");
            }
            return;
        case ND_GVAR:
            gen_lval(node);
            if (node->type->ty == ARRAY) {
                return;
            }
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
            loopval = labelcnt;
            labelcnt += 1;
            gen(node->lhs);
            printf("  test rax, rax\n");  // ZF = (rax != 0)
            printf("  je .L%d\n", loopval);  // je: ZFが1ならjump <=> raxが0ならjump
            gen(node->rhs);
            printf("  test rax, rax\n");
            printf(".L%d:\n", loopval);
            printf("  setne al\n");  // !ZFを格納
            printf("  movzb rax, al\n");
            return;
        case ND_OR:
            loopval = labelcnt;
            labelcnt += 1;
            gen(node->lhs);
            printf("  test rax, rax\n");
            printf("  jne .L%d\n", loopval); // jne: ZFが0ならjump <=> raxが0でないならjump
            gen(node->rhs);
            printf("  test rax, rax\n");
            printf(".L%d:\n", loopval);
            printf("  setne al\n");
            printf("  movzb rax, al\n");
            return;
        case ND_DECLARATION:
            gen(node->lhs);
            return;
        case ND_NEG:
            gen(node->lhs);
            printf("  neg rax\n");
            return;
        case ND_NOT:
            gen(node->lhs);
            printf("  test rax, rax\n");
            printf("  sete al\n");
            printf("  movzb rax, al\n");
            return;
        case ND_BITNOT:
            gen(node->lhs);
            printf("  not rax\n");
            return;
        case ND_INCR:
            if (node->lhs) {
                gen_lval(node->lhs);
                if (node->type->ty == PTR) {
                    printf("  add qword ptr [rax], %d\n", sizeof_parse(node->type->ptr_to));
                    printf("  mov rax, qword ptr [rax]\n");
                } else {
                    size = sizeof_parse(node->type);
                    if (size == 1) {
                        printf("  add byte ptr [rax], 1\n");
                        printf("  movzx rax, byte ptr [rax]\n");
                    } else if (size == 4) {
                        printf("  add dword ptr [rax], 1\n");
                        printf("  movsx rax, dword ptr [rax]\n");
                    } else {
                        printf("  add qword ptr [rax], 1\n");
                        printf("  mov rax, qword ptr [rax]\n");
                    }
                }
            } else {
                gen_lval(node->rhs);
                if (node->type->ty == PTR) {
                    printf("  mov rdi, qword ptr [rax]\n");
                    printf("  add qword ptr [rax], %d\n", sizeof_parse(node->type->ptr_to));
                } else {
                    size = sizeof_parse(node->type);
                    if (size == 1) {
                        printf("  movzx rdi, byte ptr [rax]\n");
                        printf("  add byte ptr [rax], 1\n");
                    } else if (size == 4) {
                        printf("  movsx rdi, dword ptr [rax]\n");
                        printf("  add dword ptr [rax], 1\n");
                    } else {
                        printf("  mov rdi, qword ptr [rax]\n");
                        printf("  add qword ptr [rax], 1\n");
                    }
                }
                printf("  mov rax, rdi\n");
            }
            return;
        case ND_DECR:
            if (node->lhs) {
                gen_lval(node->lhs);
                if (node->type->ty == PTR) {
                    printf("  sub qword ptr [rax], %d\n", sizeof_parse(node->type->ptr_to));
                    printf("  mov rax, qword ptr [rax]\n");
                } else {
                    size = sizeof_parse(node->type);
                    if (size == 1) {
                        printf("  sub byte ptr [rax], 1\n");
                        printf("  movzx rax, byte ptr [rax]\n");
                    } else if (size == 4) {
                        printf("  sub dword ptr [rax], 1\n");
                        printf("  movsx rax, dword ptr [rax]\n");
                    } else {
                        printf("  sub qword ptr [rax], 1\n");
                        printf("  mov rax, qword ptr [rax]\n");
                    }
                }
            } else {
                gen_lval(node->rhs);
                if (node->type->ty == PTR) {
                    printf("  mov rdi, qword ptr [rax]\n");
                    printf("  sub qword ptr [rax], %d\n", sizeof_parse(node->type->ptr_to));
                } else {
                    size = sizeof_parse(node->type);
                    if (size == 1) {
                        printf("  movzx rdi, byte ptr [rax]\n");
                        printf("  sub byte ptr [rax], 1\n");
                    } else if (size == 4) {
                        printf("  movsx rdi, dword ptr [rax]\n");
                        printf("  sub dword ptr [rax], 1\n");
                    } else {
                        printf("  mov rdi, qword ptr [rax]\n");
                        printf("  sub qword ptr [rax], 1\n");
                    }
                }
                printf("  mov rax, rdi\n");
            }
            return;
        case ND_BREAK:
            if (break_label == 0) {
                error("ループ外でbreakはできません");
            } else if (break_label == 1) {
                break_label = labelcnt;
                labelcnt += 1;
            }
            printf("  jmp .L%d\n", break_label);
            return;
        case ND_CONTINUE:
            if (continue_label == 0) {
                error("ループ外でcontinueはできません");
            } else if (continue_label == 1) {
                continue_label = labelcnt;
                labelcnt += 1;
            }
            printf("  jmp .L%d\n", continue_label);
            return;
    }

    gen(node->lhs);
    printf("  push rax\n");
    pushcnt += 1;
    gen(node->rhs);

    printf("  mov rdi, rax\n");
    printf("  pop rax\n");
    pushcnt -= 1;
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
