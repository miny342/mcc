#include "mcc.h"
#define min(x, y) ((x) <= (y) ? (x) : (y))

Function *fn;
Block *blk;

int used_reg;

int *ir_break_label = NULL;
int *ir_continue_label = NULL;

void add_instruction(Instruction *ir) {
    push(blk->instructions, ir);
}

int new_block() {
    blk = calloc(1, sizeof(Block));
    blk->instructions = vec_new();
    push(fn->blocks, blk);
    return fn->blocks->len - 1; // new_blockのindexを返す
}

Value *reg_value(int reg) {
    Value *v = calloc(1, sizeof(Value));
    v->num = reg;
    v->ty = V_REG;
    return v;
}


Value *add_op(InstructionOP op, Value *reg1, Value *reg2) {
    Instruction *ir = calloc(1, sizeof(Instruction));
    ir->lhs = reg1;
    ir->rhs = reg2;
    ir->op = op;
    ir->lval = reg_value(used_reg++);
    add_instruction(ir);
    return ir->lval;
}

int printe(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    return vfprintf(stderr, fmt, ap);
}

void show_value(Value *v) {
    if (!v) {
        return;
    }
    if (v->ty == V_REG) {
        printe("r%d", v->num);
    } else if (v->ty == V_LVAR) {
        printe("l_%.*s", v->lvar->len, v->lvar->name);
    } else if (v->ty == V_GVAR) {
        printe("g_%.*s", v->gvar->len, v->gvar->name);
    } else if (v->ty == V_NUM) {
        printe("%d", v->num);
    } else if (v->ty == V_STR) {
        printe(".LC%d", v->num);
    } else if (v->ty == V_DEREF) {
        printe("*r%d", v->num);
    } else {
        error("show value");
    }
}

static char *opstr[] = {
    "+", "-", "*", "/", "==", "!=", "<", "<=", "r", "if", "call", "addr", "%", "<<", ">>", "&", "^", "|", "-", "!", "~", "mov", "goto"
};

void show_ir(Instruction *ir) {
    printe("  ");
    switch (ir->op) {
        case IR_RETURN:
            printe("return ");
            show_value(ir->lhs);
            break;
        case IR_MOV:
            show_value(ir->lval);
            printe(" = ");
            show_value(ir->lhs);
            break;
        case IR_ADDR:
            show_value(ir->lval);
            printe(" = &");
            show_value(ir->lhs);
            break;
        case IR_IF:
            printe("if (");
            show_value(ir->lhs);
            printe(") goto %d", *ir->to);
            break;
        case IR_GOTO:
            printe("goto %d", *ir->to);
            break;
        case IR_CALL:
            show_value(ir->lval);
            printe(" = ");
            show_value(ir->lhs);
            printe("(");
            for (int i = 0; i < ir->args->len; i++) {
                show_value(ir->args->data[i]);
                if (i < ir->args->len - 1) {
                    printe(", ");
                }
            }
            printe(")");
            break;
        default:
            show_value(ir->lval);
            printe(" = ");
            show_value(ir->lhs);
            printe(" %s ", opstr[ir->op]);
            show_value(ir->rhs);
            break;
    }
    printe("\n");
}

void show_fn(Function *fn) {
    for (int i = 0; i < fn->blocks->len; i++) {
        printe("%d:\n", i);
        Block *b = fn->blocks->data[i];
        for (int j = 0; j < b->instructions->len; j++) {
            Instruction *inst = b->instructions->data[j];
            show_ir(inst);
        }
    }
}

void gen_global_ir() {
    GVar *data;
    GVar **tmp = &data;

    Vec *functions = vec_new();

    for(; code; code = code->next) {
        if (code->type->ty == FUNC && code->node) {
            fn = calloc(1, sizeof(Function));
            fn->blocks = vec_new();
            new_block();

            used_reg = 1;

            gen_ir(code->node);

            push(functions, fn);

            printe("fn %.*s\n", code->len, code->name);
            show_fn(fn);

        } else if (code->type->ty != FUNC) {
            *tmp = code;
            tmp = &code->next;
        }
    }
    *tmp = NULL;

    // printf(".data\n");
    // for(; data; data = data->next) {
    //     if (!data->is_extern) {
    //         if (data->is_globl) {
    //             printf(".globl %.*s\n", data->len, data->name);
    //         } else if (data->is_static) {
    //             printf(".local %.*s\n", data->len, data->name);
    //         }
    //         printf("%.*s:\n", data->len, data->name);
    //         int size = data->size - gen_gvar(data->node);
    //         if (size > 0) {
    //             printf("  .zero %d\n", size);
    //         }
    //     }
    // }

    // printf(".section .rodata\n");
    // for(; strs->next; strs = strs->next) {
    //     printf(".LC%d:\n", strs->offset);
    //     printf("  .string %.*s\n", strs->tok->len, strs->tok->str);
    // }
}

// regにアドレスを入れて返す
Value *gen_lval_ir(Node *node) {
    Value *v;
    Instruction *inst;
    if(node->kind == ND_DEREF) {
        return gen_ir(node->lhs);
    }
    inst = calloc(1, sizeof(Instruction));
    inst->lval = reg_value(used_reg++);
    inst->op = IR_ADDR;
    add_instruction(inst);
    if (node->kind == ND_GVAR) {
        v = calloc(1, sizeof(Value));
        v->ty = V_GVAR;
        v->gvar = node->gvar;
        inst->lhs = v;
        return inst->lval;
    }
    if (node->kind != ND_LVAR) {
        error("代入の左辺が変数ではありません");
    }
    v = calloc(1, sizeof(Value));
    v->ty = V_LVAR;
    v->lvar = node->lvar;
    inst->lhs = v;
    return inst->lval;
}

void gen_callstack_ir(Node *node, Vec *v) {
    if (node->lhs) {
        gen_callstack_ir(node->rhs, v);
        Value *val = gen_ir(node->lhs);
        push(v, val);
    }
}

Value *gen_call_ir(Node *node) {
    if (node->kind != ND_CALL)
        error("this is not call function");

    Vec *v = vec_new();
    gen_callstack_ir(node->rhs, v);

    for(int i = 0; i < v->len / 2; i++) {
        void *d = v->data[i];
        v->data[i] = v->data[v->len - 1 - i];
        v->data[v->len - 1 - i] = d;
    }

    Instruction *inst = calloc(1, sizeof(Instruction));
    inst->op = IR_CALL;
    inst->args = v;

    Value *val;
    if (node->lhs->kind == ND_GVAR) {
        val = calloc(1, sizeof(Value));
        val->ty = V_GVAR;
        val->gvar = node->lhs->gvar;
        inst->lhs = val;
    } else {
        val = gen_ir(node->lhs);
        inst->lhs = val;
    }
    inst->lval = reg_value(used_reg++);
    add_instruction(inst);
    return inst->lval;
}

// // ジャンプするアセンブリを吐きながら必要なラベル数を返す
// int gen_switch_top(Node *node) {
//     int needlabel = 0;
//     int defaultlabel = 0;
//     while(node) {
//         if (node->kind == ND_CASE) {
//             printf("  cmp rax, %d\n", node->lhs->val);
//             printf("  je .L%d\n", labelcnt + needlabel);
//             needlabel++;
//         } else if (node->kind == ND_DEFAULT) {
//             defaultlabel = labelcnt + needlabel;
//             needlabel++;
//         }
//         node = node->rhs;
//     }
//     if (defaultlabel) {
//         printf("  jmp .L%d\n", defaultlabel);
//     } else {
//         printf("  jmp .L%d\n", labelcnt + needlabel);
//     }
//     needlabel++;
//     return needlabel;
// }

// void gen_switch(Node *node) {
//     if (node->kind != ND_SWITCH) {
//         error("switchではありません");
//     }
//     gen(node->lhs);
//     int needlabel = gen_switch_top(node->rhs);
//     int tmp_label = labelcnt;
//     labelcnt += needlabel;
//     int now_break_label = break_label;
//     break_label = labelcnt - 1;

//     node = node->rhs;
//     while(node) {
//         if (node->kind == ND_CASE || node->kind == ND_DEFAULT) {
//             printf(".L%d:\n", tmp_label);
//             tmp_label++;
//         } else {
//             gen(node->lhs);
//         }
//         node = node->rhs;
//     }
//     printf(".L%d:\n", break_label);
//     break_label = now_break_label;
// }

// // raxをアドレスとみて読む
// void gen_load_addr(int size) {
//     if (size == 8) {
//         printf("  mov rax, qword ptr [rax]\n");
//     } else if (size == 4) {
//         printf("  movsx rax, dword ptr [rax]\n");
//     } else if (size == 2) {
//         printf("  movsx rax, word ptr [rax]\n");
//     } else if (size == 1) {
//         printf("  movzx rax, byte ptr [rax]\n");
//     } else {
//         error("no reg size %d", size);
//     }
// }

// nodeから中間表現を出力
// V_REGまたはNULLを返すことを期待する
Value *gen_ir(Node *node) {
    int loopval, size, *clabel, *blabel;
    if(node == NULL) return NULL;

    int *la1, *la2;

    Value *l, *r, *t;
    Instruction *inst;
    switch (node->kind) {
        case ND_RETURN:
            l = gen_ir(node->lhs);
            inst = calloc(1, sizeof(Instruction));
            inst->op = IR_RETURN;
            inst->lhs = l;
            add_instruction(inst);
            return NULL;
        case ND_NUM:
            inst = calloc(1, sizeof(Instruction));
            inst->lhs = calloc(1, sizeof(Value));
            inst->lhs->ty = V_NUM;
            inst->lhs->num = node->val;
            inst->op = IR_MOV;
            inst->lval = reg_value(used_reg++);
            add_instruction(inst);
            return inst->lval;
        case ND_LVAR:
            l = gen_lval_ir(node);

            t = calloc(1, sizeof(Value));
            t->num = l->num;
            t->ty = V_DEREF;

            inst = calloc(1, sizeof(Instruction));
            inst->lhs = t;
            inst->op = IR_MOV;
            inst->lval = reg_value(used_reg++);
            add_instruction(inst);
            return inst->lval;
        case ND_ASSIGN:
            r = gen_ir(node->rhs);
            l = gen_lval_ir(node->lhs);

            t = calloc(1, sizeof(Value));
            t->num = l->num;
            t->ty = V_DEREF;

            inst = calloc(1, sizeof(Instruction));
            inst->lval = t;
            inst->lhs = r;
            inst->op = IR_MOV;

            add_instruction(inst);
            return r;
        case ND_WHILE:
            blabel = ir_break_label;
            clabel = ir_continue_label;

            ir_break_label = malloc(sizeof(int));
            ir_continue_label = malloc(sizeof(int));

            *ir_continue_label = new_block();

            l = gen_ir(node->lhs);

            inst = calloc(1, sizeof(Instruction));
            inst->op = IR_IF;
            inst->to = ir_break_label;
            inst->lhs = l;
            add_instruction(inst);

            new_block();

            r = gen_ir(node->rhs);

            inst = calloc(1, sizeof(Instruction));
            inst->op = IR_GOTO;
            inst->to = ir_continue_label;
            add_instruction(inst);

            *ir_break_label = new_block();

            ir_break_label = blabel;
            ir_continue_label = clabel;

            return NULL;
        case ND_IF:
            la1 = malloc(sizeof(int));

            l = gen_ir(node->lhs);
            inst = calloc(1, sizeof(Instruction));
            inst->op = IR_IF;
            inst->lhs = l;
            inst->to = la1;
            add_instruction(inst);

            new_block();

            // ?:の時の値をどう扱うか
            if (node->rhs->kind == ND_ELSE) {
                la2 = malloc(sizeof(int));
                r = gen_ir(node->rhs->lhs);
                inst = calloc(1, sizeof(Instruction));
                inst->op = IR_GOTO;
                inst->to = la2;
                add_instruction(inst);
                *la1 = new_block();
                r = gen_ir(node->rhs->rhs);
                *la2 = new_block();
            } else {
                r = gen_ir(node->rhs);
                *la1 = new_block();
            }
            return NULL;
        case ND_FOR:
            blabel = ir_break_label;
            clabel = ir_continue_label;

            la1 = malloc(sizeof(int));
            ir_break_label = malloc(sizeof(int));
            ir_continue_label = malloc(sizeof(int));

            gen_ir(node->lhs);

            *la1 = new_block();

            l = gen_ir(node->rhs->lhs);
            inst = calloc(1, sizeof(Instruction));
            inst->lhs = l;
            inst->to = ir_break_label;
            inst->op = IR_IF;
            add_instruction(inst);

            new_block();

            l = gen_ir(node->rhs->rhs->rhs);

            *ir_continue_label = new_block();

            l = gen_ir(node->rhs->rhs->lhs);
            inst = calloc(1, sizeof(Instruction));
            inst->op = IR_GOTO;
            inst->to = la1;
            add_instruction(inst);

            *ir_break_label = new_block();

            ir_break_label = blabel;
            ir_continue_label = clabel;
            return NULL;
        case ND_BLOCK:
            gen_ir(node->rhs);
            gen_ir(node->lhs);
            return NULL;
        case ND_CALL:
            return gen_call_ir(node);
        case ND_ADDR:
            l = gen_lval_ir(node->lhs);
            return l;
        case ND_DEREF:
            l = gen_ir(node->lhs);

            t = calloc(1, sizeof(Value));
            t->num = l->num;
            t->ty = V_DEREF;

            inst = calloc(1, sizeof(Instruction));
            inst->lval = reg_value(used_reg++);
            inst->lhs = t;
            inst->op = IR_MOV;
            add_instruction(inst);
            return inst->lval;
        // case ND_GVAR:
        //     gen_lval(node);
        //     if (node->type->ty == ARRAY) {
        //         return;
        //     }
        //     size = sizeof_parse(node->gvar->type);
        //     gen_load_addr(size);
        //     return;
        // case ND_STR:
        //     printf("  lea rax, .LC%d[rip]\n", node->s->offset);
        //     return;
        // case ND_AND:
        //     loopval = labelcnt;
        //     labelcnt += 1;
        //     gen(node->lhs);
        //     printf("  test rax, rax\n");  // ZF = (rax != 0)
        //     printf("  je .L%d\n", loopval);  // je: ZFが1ならjump <=> raxが0ならjump
        //     gen(node->rhs);
        //     printf("  test rax, rax\n");
        //     printf(".L%d:\n", loopval);
        //     printf("  setne al\n");  // !ZFを格納
        //     printf("  movzb rax, al\n");
        //     return;
        // case ND_OR:
        //     loopval = labelcnt;
        //     labelcnt += 1;
        //     gen(node->lhs);
        //     printf("  test rax, rax\n");
        //     printf("  jne .L%d\n", loopval); // jne: ZFが0ならjump <=> raxが0でないならjump
        //     gen(node->rhs);
        //     printf("  test rax, rax\n");
        //     printf(".L%d:\n", loopval);
        //     printf("  setne al\n");
        //     printf("  movzb rax, al\n");
        //     return;
        case ND_DECLARATION:
            gen_ir(node->lhs);
            return NULL;
        // case ND_NEG:
        //     gen(node->lhs);
        //     printf("  neg rax\n");
        //     return;
        // case ND_NOT:
        //     gen(node->lhs);
        //     printf("  test rax, rax\n");
        //     printf("  sete al\n");
        //     printf("  movzb rax, al\n");
        //     return;
        // case ND_BITNOT:
        //     gen(node->lhs);
        //     printf("  not rax\n");
        //     return;
        // case ND_INCR:
        //     if (node->lhs) {
        //         gen_lval(node->lhs);
        //         if (node->type->ty == PTR) {
        //             printf("  add qword ptr [rax], %d\n", sizeof_parse(node->type->ptr_to));
        //             printf("  mov rax, qword ptr [rax]\n");
        //         } else {
        //             size = sizeof_parse(node->type);
        //             if (size == 1) {
        //                 printf("  add byte ptr [rax], 1\n");
        //                 printf("  movzx rax, byte ptr [rax]\n");
        //             } else if (size == 4) {
        //                 printf("  add dword ptr [rax], 1\n");
        //                 printf("  movsx rax, dword ptr [rax]\n");
        //             } else if (size == 8) {
        //                 printf("  add qword ptr [rax], 1\n");
        //                 printf("  mov rax, qword ptr [rax]\n");
        //             } else if (size == 2) {
        //                 printf("  add word ptr [rax], 1\n");
        //                 printf("  movsx rax, word ptr [rax]\n");
        //             } else {
        //                 error("no reg size %d", size);
        //             }
        //         }
        //     } else {
        //         gen_lval(node->rhs);
        //         if (node->type->ty == PTR) {
        //             printf("  mov rdi, qword ptr [rax]\n");
        //             printf("  add qword ptr [rax], %d\n", sizeof_parse(node->type->ptr_to));
        //         } else {
        //             size = sizeof_parse(node->type);
        //             if (size == 1) {
        //                 printf("  movzx rdi, byte ptr [rax]\n");
        //                 printf("  add byte ptr [rax], 1\n");
        //             } else if (size == 4) {
        //                 printf("  movsx rdi, dword ptr [rax]\n");
        //                 printf("  add dword ptr [rax], 1\n");
        //             } else if (size == 8) {
        //                 printf("  mov rdi, qword ptr [rax]\n");
        //                 printf("  add qword ptr [rax], 1\n");
        //             } else if (size == 2) {
        //                 printf("  movsx rdi, word ptr [rax]\n");
        //                 printf("  add word ptr [rax], 1\n");
        //             } else {
        //                 error("no reg size %d", size);
        //             }
        //         }
        //         printf("  mov rax, rdi\n");
        //     }
        //     return;
        // case ND_DECR:
        //     if (node->lhs) {
        //         gen_lval(node->lhs);
        //         if (node->type->ty == PTR) {
        //             printf("  sub qword ptr [rax], %d\n", sizeof_parse(node->type->ptr_to));
        //             printf("  mov rax, qword ptr [rax]\n");
        //         } else {
        //             size = sizeof_parse(node->type);
        //             if (size == 1) {
        //                 printf("  sub byte ptr [rax], 1\n");
        //                 printf("  movzx rax, byte ptr [rax]\n");
        //             } else if (size == 4) {
        //                 printf("  sub dword ptr [rax], 1\n");
        //                 printf("  movsx rax, dword ptr [rax]\n");
        //             } else if (size == 8) {
        //                 printf("  sub qword ptr [rax], 1\n");
        //                 printf("  mov rax, qword ptr [rax]\n");
        //             } else if (size == 2) {
        //                 printf("  sub word ptr [rax], 1\n");
        //                 printf("  movsx rax, word ptr [rax]\n");
        //             } else {
        //                 error("no reg size %d", size);
        //             }
        //         }
        //     } else {
        //         gen_lval(node->rhs);
        //         if (node->type->ty == PTR) {
        //             printf("  mov rdi, qword ptr [rax]\n");
        //             printf("  sub qword ptr [rax], %d\n", sizeof_parse(node->type->ptr_to));
        //         } else {
        //             size = sizeof_parse(node->type);
        //             if (size == 1) {
        //                 printf("  movzx rdi, byte ptr [rax]\n");
        //                 printf("  sub byte ptr [rax], 1\n");
        //             } else if (size == 4) {
        //                 printf("  movsx rdi, dword ptr [rax]\n");
        //                 printf("  sub dword ptr [rax], 1\n");
        //             } else if (size == 8) {
        //                 printf("  mov rdi, qword ptr [rax]\n");
        //                 printf("  sub qword ptr [rax], 1\n");
        //             } else if (size == 2) {
        //                 printf("  movsx rdi, word ptr [rax]\n");
        //                 printf("  sub word ptr [rax], 1\n");
        //             } else {
        //                 error("no reg size %d", size);
        //             }
        //         }
        //         printf("  mov rax, rdi\n");
        //     }
        //     return;
        // case ND_BREAK:
        //     if (break_label == 0) {
        //         error("ループ外でbreakはできません");
        //     } else if (break_label == 1) {
        //         break_label = labelcnt;
        //         labelcnt += 1;
        //     }
        //     printf("  jmp .L%d\n", break_label);
        //     return;
        // case ND_CONTINUE:
        //     if (continue_label == 0) {
        //         error("ループ外でcontinueはできません");
        //     } else if (continue_label == 1) {
        //         continue_label = labelcnt;
        //         labelcnt += 1;
        //     }
        //     printf("  jmp .L%d\n", continue_label);
        //     return;
        // case ND_SWITCH:
        //     gen_switch(node);
        //     return;
        // case ND_DO:
        //     loopval = labelcnt;
        //     labelcnt += 2;
        //     blabel = break_label;
        //     clabel = continue_label;
        //     break_label = loopval + 1;
        //     continue_label = loopval;
        //     printf(".L%d:\n", loopval);
        //     gen(node->rhs);
        //     gen(node->lhs);
        //     printf("  cmp rax, 0\n");
        //     printf("  jne .L%d\n", loopval);
        //     printf(".L%d:\n", loopval + 1);
        //     break_label = blabel;
        //     continue_label = clabel;
        //     return;
    }

    l = gen_ir(node->lhs);
    r = gen_ir(node->rhs);

    switch(node->kind) {
        case ND_ADD:
            return add_op(IR_ADD, l, r);
        case ND_SUB:
            return add_op(IR_SUB, l, r);
        case ND_MUL:
            return add_op(IR_MUL, l, r);
        case ND_DIV:
            return add_op(IR_DIV, l, r);
        case ND_EQ:
            return add_op(IR_EQ, l, r);
        case ND_NE:
            return add_op(IR_NE, l, r);
        case ND_LE:
            return add_op(IR_LE, l, r);
        case ND_LT:
            return add_op(IR_LT, l, r);
        case ND_REMINDER:
            return add_op(IR_REMINDER, l, r);
        case ND_LSHIFT:
            return add_op(IR_LSHIFT, l, r);
        case ND_RSHIFT:
            return add_op(IR_RSHIFT, l, r);
        case ND_BITAND:
            return add_op(IR_BITAND, l, r);
        case ND_BITOR:
            return add_op(IR_BITOR, l, r);
        case ND_BITXOR:
            return add_op(IR_BITXOR, l, r);
    }
    error("ir error %d", node->kind);
}
