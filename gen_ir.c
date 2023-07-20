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
    "+", "-", "*", "/", "==", "!=", "<", "<=", "r", "ifeq", "ifne", "call", "&", "%", "<<", ">>", "&", "^", "|", "-", "!", "~", "mov", "goto"
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
        case IR_IF_EQ_0:
            printe("if (");
            show_value(ir->lhs);
            printe(" == 0) goto %d", *ir->to);
            break;
        case IR_IF_NE_0:
            printe("if (");
            show_value(ir->lhs);
            printe(" != 0) goto %d", *ir->to);
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
        case IR_PHI:
            show_value(ir->lval);
            printe(" = Φ(");
            show_value(ir->lhs);
            printe(", ");
            show_value(ir->rhs);
            printe(")");
            break;
        case IR_ADDR:
        case IR_NEG:
        case IR_NOT:
        case IR_BITNOT:
            show_value(ir->lval);
            printe(" = %s", opstr[ir->op]);
            show_value(ir->lhs);
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

// Vec<Function>
Vec *gen_global_ir() {
    GVar *data = code;
    GVar **tmp = &code;

    Vec *functions = vec_new();

    Value *v;
    Instruction *inst;
    for(; data; data = data->next) {
        if (data->type->ty == FUNC && data->node) {
            fn = calloc(1, sizeof(Function));
            fn->gvar = data;
            fn->blocks = vec_new();
            new_block();

            used_reg = 0;

            gen_ir(data->node);

            v = calloc(1, sizeof(Value));
            v->num = 0;
            v->ty = V_NUM;

            inst = calloc(1, sizeof(Instruction));
            inst->lhs = v;
            inst->op = IR_RETURN;
            add_instruction(inst);

            push(functions, fn);

            printe("fn %.*s\n", data->len, data->name);
            show_fn(fn);
        } else if (data->type->ty != FUNC) {
            *tmp = data;
            tmp = &data->next;
        }
    }
    return functions;
}

// regにアドレスを入れて返す
Value *gen_lval_ir(Node *node) {
    Value *v, *res;
    if(node->kind == ND_DEREF) {
        return gen_ir(node->lhs);
    }
    v = calloc(1, sizeof(Value));
    res = add_op(IR_ADDR, v, NULL);
    if (node->kind == ND_GVAR) {
        v->ty = V_GVAR;
        v->gvar = node->gvar;
        return res;
    }
    if (node->kind != ND_LVAR) {
        error("代入の左辺が変数ではありません");
    }
    v->ty = V_LVAR;
    v->lvar = node->lvar;
    return res;
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


Vec *gen_switch_top_ir(Node *node, Value *reg) {
    Vec *labels = vec_new();
    int *defaultlabel = NULL;
    int *endlabel = malloc(sizeof(int));
    Value *v;
    Instruction *inst;
    while(node) {
        if (node->kind == ND_CASE) {
            v = calloc(1, sizeof(Value));
            v->num = node->lhs->val;
            v->ty = V_NUM;
            v = add_op(IR_EQ, reg, v);
            inst = calloc(1, sizeof(Instruction));
            inst->op = IR_IF_NE_0;
            inst->to = malloc(sizeof(int));
            inst->lhs = v;
            add_instruction(inst);
            push(labels, inst->to);
            new_block();
        } else if (node->kind == ND_DEFAULT) {
            defaultlabel = malloc(sizeof(int));
        }
        node = node->rhs;
    }
    if (defaultlabel) {
        inst = calloc(1, sizeof(Instruction));
        inst->op = IR_GOTO;
        inst->to = defaultlabel;
        add_instruction(inst);
        push(labels, defaultlabel);
        new_block();
    }
    inst = calloc(1, sizeof(Instruction));
    inst->op = IR_GOTO;
    inst->to = endlabel;
    add_instruction(inst);
    push(labels, endlabel);
    new_block();
    return labels;
}

void gen_switch_ir(Node *node) {
    if (node->kind != ND_SWITCH) {
        error("switchではありません");
    }
    Value *v = gen_ir(node->lhs);
    Vec *needlabel = gen_switch_top_ir(node->rhs, v);
    int *maybe_defaultlabel = needlabel->data[needlabel->len - 2];
    int *endlabel = needlabel->data[needlabel->len - 1];

    int *now_break_label = ir_break_label;
    ir_break_label = endlabel;

    node = node->rhs;
    int cnt = 0;
    while(node) {
        if (node->kind == ND_CASE) {
            int *i = needlabel->data[cnt];
            *i = new_block();
            cnt++;
        } else if (node->kind == ND_DEFAULT) {
            *maybe_defaultlabel = new_block();
        } else {
            gen_ir(node->lhs);
        }
        node = node->rhs;
    }
    *endlabel = new_block();
    ir_break_label = now_break_label;
}

// nodeから中間表現を出力
// V_REGまたはNULLを返すことを期待する
Value *gen_ir(Node *node) {
    int tmp1, tmp2, *clabel, *blabel;
    if(node == NULL) return NULL;

    int *la1, *la2;

    Value *l, *r, *t;
    Instruction *inst;
    switch (node->kind) {
        case ND_RETURN:
            inst = calloc(1, sizeof(Instruction));
            inst->lhs = gen_ir(node->lhs);
            inst->op = IR_RETURN;
            add_instruction(inst);
            return NULL;
        case ND_NUM:
            t = calloc(1, sizeof(Value));
            t->ty = V_NUM;
            t->num = node->val;
            return add_op(IR_MOV, t, NULL);
        case ND_LVAR:
            l = gen_lval_ir(node);

            if (node->lvar->type->ty == ARRAY) {
                return l;
            }

            t = calloc(1, sizeof(Value));
            t->num = l->num;
            t->ty = V_DEREF;

            return add_op(IR_MOV, t, NULL);
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
            inst->op = IR_IF_EQ_0;
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
            inst->op = IR_IF_EQ_0;
            inst->lhs = l;
            inst->to = la1;
            add_instruction(inst);

            new_block();

            if (node->rhs->kind == ND_ELSE) {
                la2 = malloc(sizeof(int));
                l = gen_ir(node->rhs->lhs);
                inst = calloc(1, sizeof(Instruction));
                inst->op = IR_GOTO;
                inst->to = la2;
                add_instruction(inst);
                *la1 = new_block();
                r = gen_ir(node->rhs->rhs);
                *la2 = new_block();
                if (l && r) return add_op(IR_PHI, l, r);
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
            inst->op = IR_IF_EQ_0;
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

            return add_op(IR_MOV, t, NULL);
        case ND_GVAR:
            l = gen_lval_ir(node);

            if (node->gvar->type->ty == ARRAY) {
                return l;
            }

            t = calloc(1, sizeof(Value));
            t->num = l->num;
            t->ty = V_DEREF;

            return add_op(IR_MOV, t, NULL);
        case ND_STR:
            t = calloc(1, sizeof(Value));
            t->ty = V_STR;
            t->num = node->s->offset;

            return add_op(IR_MOV, t, NULL);
        case ND_AND:
            la1 = malloc(sizeof(int));
            la2 = malloc(sizeof(int));

            l = gen_ir(node->lhs);
            inst = calloc(1, sizeof(Instruction));
            inst->op = IR_IF_EQ_0;
            inst->lhs = l;
            inst->to = la1;
            add_instruction(inst);
            new_block();

            l = gen_ir(node->rhs);
            inst = calloc(1, sizeof(Instruction));
            inst->op = IR_IF_EQ_0;
            inst->lhs = l;
            inst->to = la1;
            add_instruction(inst);

            new_block();

            t = calloc(1, sizeof(Value));
            t->num = 1;
            t->ty = V_NUM;

            l = add_op(IR_MOV, t, NULL);

            inst = calloc(1, sizeof(Instruction));
            inst->op = IR_GOTO;
            inst->to = la2;
            add_instruction(inst);

            *la1 = new_block();

            t = calloc(1, sizeof(Value));
            t->num = 0;
            t->ty = V_NUM;

            r = add_op(IR_MOV, t, NULL);

            *la2 = new_block();

            return add_op(IR_PHI, l, r);
        case ND_OR:
            la1 = malloc(sizeof(int));
            la2 = malloc(sizeof(int));

            l = gen_ir(node->lhs);
            inst = calloc(1, sizeof(Instruction));
            inst->op = IR_IF_NE_0;
            inst->lhs = l;
            inst->to = la1;
            add_instruction(inst);
            new_block();

            l = gen_ir(node->rhs);
            inst = calloc(1, sizeof(Instruction));
            inst->op = IR_IF_NE_0;
            inst->lhs = l;
            inst->to = la1;
            add_instruction(inst);

            new_block();

            t = calloc(1, sizeof(Value));
            t->num = 0;
            t->ty = V_NUM;

            l = add_op(IR_MOV, t, NULL);

            inst = calloc(1, sizeof(Instruction));
            inst->op = IR_GOTO;
            inst->to = la2;
            add_instruction(inst);

            *la1 = new_block();

            t = calloc(1, sizeof(Value));
            t->num = 1;
            t->ty = V_NUM;

            r = add_op(IR_MOV, t, NULL);

            *la2 = new_block();

            return add_op(IR_PHI, l, r);
        case ND_DECLARATION:
            gen_ir(node->lhs);
            return NULL;
        case ND_NEG:
            return add_op(IR_NEG, gen_ir(node->lhs), NULL);
        case ND_NOT:
            return add_op(IR_NOT, gen_ir(node->lhs), NULL);
        case ND_BITNOT:
            return add_op(IR_BITNOT, gen_ir(node->lhs), NULL);
        case ND_INCR:
            if (node->lhs) {
                l = gen_lval_ir(node->lhs);
                t = calloc(1, sizeof(Value));
                t->num = l->num;
                t->ty = V_DEREF;

                inst = calloc(1, sizeof(Instruction));
                inst->op = IR_ADD;
                inst->lval = t;
                inst->lhs = t;

                r = calloc(1, sizeof(Value));
                r->ty = V_NUM;
                if (node->type->ty == PTR) {
                    r->num = sizeof_parse(node->type->ptr_to);
                } else {
                    r->num = 1;
                }

                inst->rhs = r;
                add_instruction(inst);
                return add_op(IR_MOV, t, NULL);
            } else {
                l = gen_lval_ir(node->rhs);
                t = calloc(1, sizeof(Value));
                t->num = l->num;
                t->ty = V_DEREF;

                r = add_op(IR_MOV, t, NULL);

                inst = calloc(1, sizeof(Instruction));
                inst->op = IR_ADD;
                inst->lval = t;
                inst->lhs = t;

                l = calloc(1, sizeof(Value));
                l->ty = V_NUM;
                if (node->type->ty == PTR) {
                    l->num = sizeof_parse(node->type->ptr_to);
                } else {
                    l->num = 1;
                }

                inst->rhs = l;
                add_instruction(inst);
                return r;
            }
        case ND_DECR:
            if (node->lhs) {
                l = gen_lval_ir(node->lhs);
                t = calloc(1, sizeof(Value));
                t->num = l->num;
                t->ty = V_DEREF;

                inst = calloc(1, sizeof(Instruction));
                inst->op = IR_SUB;
                inst->lval = t;
                inst->lhs = t;

                r = calloc(1, sizeof(Value));
                r->ty = V_NUM;
                if (node->type->ty == PTR) {
                    r->num = sizeof_parse(node->type->ptr_to);
                } else {
                    r->num = 1;
                }

                inst->rhs = r;
                add_instruction(inst);
                return add_op(IR_MOV, t, NULL);
            } else {
                l = gen_lval_ir(node->rhs);
                t = calloc(1, sizeof(Value));
                t->num = l->num;
                t->ty = V_DEREF;

                r = add_op(IR_MOV, t, NULL);

                inst = calloc(1, sizeof(Instruction));
                inst->op = IR_SUB;
                inst->lval = t;
                inst->lhs = t;

                l = calloc(1, sizeof(Value));
                l->ty = V_NUM;
                if (node->type->ty == PTR) {
                    l->num = sizeof_parse(node->type->ptr_to);
                } else {
                    l->num = 1;
                }

                inst->rhs = l;
                add_instruction(inst);
                return r;
            }
        case ND_BREAK:
            if (ir_break_label == NULL) {
                error("ループ外でbreakはできません");
            }
            inst = calloc(1, sizeof(Instruction));
            inst->op = IR_GOTO;
            inst->to = ir_break_label;
            add_instruction(inst);
            new_block();
            return NULL;
        case ND_CONTINUE:
            if (ir_continue_label == NULL) {
                error("ループ外でcontinueはできません");
            }
            inst = calloc(1, sizeof(Instruction));
            inst->op = IR_GOTO;
            inst->to = ir_continue_label;
            add_instruction(inst);
            new_block();
            return NULL;
        case ND_SWITCH:
            gen_switch_ir(node);
            return NULL;
        case ND_DO:
            blabel = ir_break_label;
            clabel = ir_continue_label;

            ir_break_label = malloc(sizeof(int));
            ir_continue_label = malloc(sizeof(int));

            *ir_continue_label = new_block();
            gen_ir(node->rhs);
            l = gen_ir(node->lhs);
            inst = calloc(1, sizeof(Instruction));
            inst->op = IR_IF_NE_0;
            inst->to = ir_continue_label;
            inst->lhs = l;
            add_instruction(inst);

            *ir_break_label = new_block();

            ir_break_label = blabel;
            ir_continue_label = clabel;
            return NULL;
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
