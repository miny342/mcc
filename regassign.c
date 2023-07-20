#include "mcc.h"

// 現状は変数をレジスタに割り当てないため問題ない
Vec *calc_reginfo(Function *f) {
    Vec *livenesses = vec_new();

    for (int i = 0; i < f->blocks->len; i++) {
        Block *b = f->blocks->data[i];
        for (int j = 0; j < b->instructions->len; j++) {
            Instruction *inst = b->instructions->data[j];
            RegisterInfo *liv;
            if (inst->lval && inst->lval->ty == V_REG) {
                if (livenesses->len != inst->lval->num) {
                    error("unexpected reg error");
                }
                liv = malloc(sizeof(RegisterInfo));
                liv->startblock = i;
                liv->endblock = i;
                liv->startline = j;
                liv->endline = j;
                liv->is_used_by_phi = 0;
                liv->phi_to = 0;
                liv->num = inst->lval->num;
                push(livenesses, liv);
            }
            if (inst->lval && inst->lval->ty == V_DEREF) {
                liv = at(livenesses, inst->lval->num);
                liv->endblock = i;
                liv->endline = j;
            }
            if (inst->lhs && (inst->lhs->ty == V_REG || inst->lhs->ty == V_DEREF)) {
                liv = at(livenesses, inst->lhs->num);
                liv->endblock = i;
                liv->endline = j;
            }
            if (inst->rhs && (inst->rhs->ty == V_REG || inst->rhs->ty == V_DEREF)) {
                liv = at(livenesses, inst->rhs->num);
                liv->endblock = i;
                liv->endline = j;
            }
            if (inst->op == IR_CALL) {
                Value *v;
                for (int k = 0; k < inst->args->len; k++) {
                    v = inst->args->data[k];
                    if (v->ty != V_REG) {
                        error("call not vreg");
                    }
                    liv = at(livenesses, v->num);
                    liv->endblock = i;
                    liv->endline = j;
                }
                v = inst->lval;
                if (v->ty == V_REG) {
                    liv = at(livenesses, v->num);
                    liv->endblock = i;
                    liv->endblock = j;
                }
            }
            if (inst->op == IR_PHI) {
                // 左辺
                liv = at(livenesses, inst->lhs->num);
                if (liv->is_used_by_phi) {
                    error("unimplemented double use phi");
                }
                liv->endblock = i;
                liv->endline = j;
                liv->is_used_by_phi = 1;
                liv->phi_to = inst->lval->num;
                // 右辺
                liv = at(livenesses, inst->rhs->num);
                if (liv->is_used_by_phi) {
                    error("unimplemented double use phi");
                }
                liv->endblock = i;
                liv->endline = j;
                liv->is_used_by_phi = 1;
                liv->phi_to = inst->lval->num;
            }
        }
    }
    return livenesses;
}

void show_reginfo(Vec *reginfo) {
    fprintf(stderr, "start show reginfo\n");
    for (int i = 0; i < reginfo->len; i++) {
        RegisterInfo *info = reginfo->data[i];
        fprintf(stderr, "RegisterInfo %d {\n  startblock: %d\n  endblock: %d\n  startline: %d\n  endline: %d\n  is_used_by_phi: %d\n  phi_to: %d\n  num: %d\n}\n", i, info->startblock, info->endblock, info->startline, info->endline, info->is_used_by_phi, info->phi_to, info->num);
    }
    fprintf(stderr, "end\n");
}


void gen_function(Vec *funcs) {
    Function *f = at(funcs, 0);
    show_reginfo(calc_reginfo(f));
}
