#include "mcc.h"
#define min(x, y) ((x) <= (y) ? (x) : (y))

char *byte_reg[] = {"dil", "sil", "dl", "cl", "r8b", "r9b"};
char *word_reg[] = {"di", "si", "dx", "cx", "r8w", "r9w"};
char *dword_reg[] = {"edi", "esi", "edx", "ecx", "r8d", "r9d"};
char *qword_reg[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};

char *ptr_name(int size) {
    switch (size) {
        case 1:
            return "byte ptr";
        case 2:
            return "word ptr";
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
        case 2:
            return word_reg;
        case 4:
            return dword_reg;
        case 8:
            return qword_reg;
        default:
            error("regs is not implemented %d\n", size);
    }
}

char *acc_regs(int size) {
    switch (size) {
        case 1:
            return "al";
        case 2:
            return "ax";
        case 4:
            return "eax";
        case 8:
            return "rax";
        default:
            error("regs is not implemented %d\n", size);
    }
}

void gen_global() {
    // アセンブリの前半部分を出力
    printf(".intel_syntax noprefix\n");

    // コンパイラのバグ検出用
    // rspが16の倍数か検証し、そうでない場合exit(2)を呼ぶ
    printf("__checkrsp:\n  push rdi\n  mov rdi, rsp\n  and rdi, 15\n  test rdi, rdi\n  je .checkrsp\n  mov rdi, 2\n  call exit\n.checkrsp:\n  pop rdi\n  ret\n");

    GVar *data;
    GVar **tmp = &data;

    for(; code; code = code->next) {
        if (code->type->ty == FUNC && code->node) {
            Type *type = code->type;
            if (!code->is_static) {
                printf(".globl %.*s\n", code->len, code->name);
            }
            printf("%.*s:\n", code->len, code->name);

            // prologue
            printf("  push rbp\n");
            printf("  mov rbp, rsp\n");

            int sub = (code->offset) + (16 - code->offset % 16) % 16;
            if (sub > 0)
                printf("  sub rsp, %d\n", sub);

            Vec *args = type->args;
            int argnum = args->len;
            if (argnum > 0) {
                Type *ty = args->data[argnum - 1];
                if (ty->ty == VA_ARGS || argnum > 6) {
                    argnum = 6;
                }
            }
            int offset = argnum * 8;
            for(int i = 0; i < argnum; i++) {
                printf("  mov qword ptr [rbp-%d], %s\n", offset, qword_reg[i]);
                offset -= 8;
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
        if (!data->is_extern) {
            if (data->is_globl) {
                printf(".globl %.*s\n", data->len, data->name);
            } else if (data->is_static) {
                printf(".local %.*s\n", data->len, data->name);
            }
            printf("%.*s:\n", data->len, data->name);
            int size = data->size - gen_gvar(data->node);
            if (size > 0) {
                printf("  .zero %d\n", size);
            }
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
                } else if (size == 1) {
                    printf("  .byte");
                } else if (size == 2) {
                    printf("  .value");
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
    int use_stack;
    if(node->lhs) {
        use_stack = gen_callstack(node->rhs, num + 1);
        gen(node->lhs);
        printf("  push rax\n");
        pushcnt++;
        return use_stack;
    } else {
        use_stack = num > 6 ? num - 6 : 0;
        if ((pushcnt - use_stack) & 1) {
            printf("  sub rsp, 8\n");
            pushcnt++;
            return use_stack + 1;
        }
        return use_stack;
    }
}

void gen_callregister(Node *node) {
    int num = 0;
    node = node->rhs;
    while(node && num < 6) {
        printf("  pop %s\n", qword_reg[num]);
        pushcnt--;
        num++;
        node = node->rhs;
    }
}

void gen_call(Node *node) {
    if (node->kind != ND_CALL)
        error("this is not call function");

    if (node->lhs->kind == ND_GVAR && node->lhs->gvar->len == 10 && !strncmp(node->lhs->gvar->name, "__va_start", 10)) {
        int ap_offset = node->rhs->lhs->lvar->offset;
        int arg_offset = node->rhs->rhs->lhs->lhs->lvar->offset;
        if (arg_offset > 0) {
            printf("  mov dword ptr [rbp-%d], %d\n", ap_offset, 56 - arg_offset);
            printf("  lea rax, [rbp+16]\n");
        } else {
            printf("  mov dword ptr [rbp-%d], %d\n", ap_offset, 48);
            printf("  lea rax, [rbp+%d]\n", 8 - arg_offset);
        }
        printf("  mov qword ptr [rbp-%d], rax\n", ap_offset - 8);
        printf("  mov dword ptr [rbp-%d], %d\n", ap_offset - 4, 0);
        printf("  lea rax, [rbp-48]\n");
        printf("  mov qword ptr [rbp-%d], rax\n", ap_offset - 16);
        return;
    }

    int stack = gen_callstack(node->rhs, 0);

    if (node->lhs->kind == ND_GVAR) {
        gen_callregister(node->rhs);
        printf("  xor rax, rax\n");
        printf("  call __checkrsp\n");  // 呼び出し前にrspの検証をする
        printf("  call %.*s\n", node->lhs->gvar->len, node->lhs->gvar->name);
    } else {
        gen(node->lhs);
        gen_callregister(node->rhs);
        printf("  mov r10, rax\n");
        printf("  xor rax, rax\n");
        printf("  call __checkrsp\n");
        printf("  call r10\n");
    }
    if (stack > 0) {
        printf("  add rsp, %d\n", stack * 8);
        pushcnt -= stack;
    }
}

// ジャンプするアセンブリを吐きながら必要なラベル数を返す
int gen_switch_top(Node *node) {
    int needlabel = 0;
    int defaultlabel = 0;
    while(node) {
        if (node->kind == ND_CASE) {
            printf("  cmp rax, %d\n", node->lhs->val);
            printf("  je .L%d\n", labelcnt + needlabel);
            needlabel++;
        } else if (node->kind == ND_DEFAULT) {
            defaultlabel = labelcnt + needlabel;
            needlabel++;
        }
        node = node->rhs;
    }
    if (defaultlabel) {
        printf("  jmp .L%d\n", defaultlabel);
    } else {
        printf("  jmp .L%d\n", labelcnt + needlabel);
    }
    needlabel++;
    return needlabel;
}

void gen_switch(Node *node) {
    if (node->kind != ND_SWITCH) {
        error("switchではありません");
    }
    gen(node->lhs);
    int needlabel = gen_switch_top(node->rhs);
    int tmp_label = labelcnt;
    labelcnt += needlabel;
    int now_break_label = break_label;
    break_label = labelcnt - 1;

    node = node->rhs;
    while(node) {
        if (node->kind == ND_CASE || node->kind == ND_DEFAULT) {
            printf(".L%d:\n", tmp_label);
            tmp_label++;
        } else {
            gen(node->lhs);
        }
        node = node->rhs;
    }
    printf(".L%d:\n", break_label);
    break_label = now_break_label;
}

// raxをアドレスとみて読む
void gen_load_addr(int size) {
    if (size == 8) {
        printf("  mov rax, qword ptr [rax]\n");
    } else if (size == 4) {
        printf("  movsx rax, dword ptr [rax]\n");
    } else if (size == 2) {
        printf("  movsx rax, word ptr [rax]\n");
    } else if (size == 1) {
        printf("  movzx rax, byte ptr [rax]\n");
    } else {
        error("no reg size %d", size);
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
            gen_load_addr(size);
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
            } else if (size == 8) {
                printf("  mov qword ptr [rdi], rax\n");
            } else if (size == 2) {
                printf("  mov word ptr [rdi], ax\n");
            } else {
                error("no reg size %d", size);
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
            gen_load_addr(size);
            return;
        case ND_GVAR:
            gen_lval(node);
            if (node->type->ty == ARRAY) {
                return;
            }
            size = sizeof_parse(node->gvar->type);
            gen_load_addr(size);
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
                    } else if (size == 8) {
                        printf("  add qword ptr [rax], 1\n");
                        printf("  mov rax, qword ptr [rax]\n");
                    } else if (size == 2) {
                        printf("  add word ptr [rax], 1\n");
                        printf("  movsx rax, word ptr [rax]\n");
                    } else {
                        error("no reg size %d", size);
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
                    } else if (size == 8) {
                        printf("  mov rdi, qword ptr [rax]\n");
                        printf("  add qword ptr [rax], 1\n");
                    } else if (size == 2) {
                        printf("  movsx rdi, word ptr [rax]\n");
                        printf("  add word ptr [rax], 1\n");
                    } else {
                        error("no reg size %d", size);
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
                    } else if (size == 8) {
                        printf("  sub qword ptr [rax], 1\n");
                        printf("  mov rax, qword ptr [rax]\n");
                    } else if (size == 2) {
                        printf("  sub word ptr [rax], 1\n");
                        printf("  movsx rax, word ptr [rax]\n");
                    } else {
                        error("no reg size %d", size);
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
                    } else if (size == 8) {
                        printf("  mov rdi, qword ptr [rax]\n");
                        printf("  sub qword ptr [rax], 1\n");
                    } else if (size == 2) {
                        printf("  movsx rdi, word ptr [rax]\n");
                        printf("  sub word ptr [rax], 1\n");
                    } else {
                        error("no reg size %d", size);
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
        case ND_SWITCH:
            gen_switch(node);
            return;
        case ND_DO:
            loopval = labelcnt;
            labelcnt += 2;
            blabel = break_label;
            clabel = continue_label;
            break_label = loopval + 1;
            continue_label = loopval;
            printf(".L%d:\n", loopval);
            gen(node->rhs);
            gen(node->lhs);
            printf("  cmp rax, 0\n");
            printf("  jne .L%d\n", loopval);
            printf(".L%d:\n", loopval + 1);
            break_label = blabel;
            continue_label = clabel;
            return;
    }
    gen(node->rhs);
    printf("  push rax\n");
    pushcnt += 1;

    gen(node->lhs);
    printf("  pop rdi\n");
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
