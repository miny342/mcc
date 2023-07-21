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
                    liv->endline = j;
                } else {
                    error("not implemented call v.ty != VREG");
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

int reg_assign(Function *f, Vec *reg_info, int *virtreg_to_realreg) {
    RegisterInfo *realreg[REG_NUM] = {};

    int max_used_reg = 0;
    int phi_reg = ERR;

    for (int block_index = 0; block_index < f->blocks->len; block_index++) {
        Block *bl = f->blocks->data[block_index];
        for (int instruction_index = 0; instruction_index < bl->instructions->len; instruction_index++) {
            Instruction *inst = bl->instructions->data[instruction_index];

            int lhs_reg = ERR;
            int rhs_reg = ERR;
            int lval_reg = ERR;

            // reginfoでphiの情報はあらかじめ持っておくので、phi_regに格納されている
            if (inst->op == IR_PHI) {
                lval_reg = phi_reg;
                phi_reg = ERR;
            }

            // argsを解放できるなら解放
            if (inst->op == IR_CALL) {
                for (int i = 0; i < inst->args->len; i++) {
                    Value *v = inst->args->data[i];
                    RegisterInfo *arg_info = at(reg_info, v->num);
                    if (arg_info->endblock == block_index && arg_info->endline == instruction_index) {
                        realreg[virtreg_to_realreg[arg_info->num]] = NULL;
                    }
                }
            }

            // lhsに使用されるレジスタの使用する値を取得し、解放できるなら解放
            if (inst->lhs && (inst->lhs->ty == V_REG || inst->lhs->ty == V_DEREF)) {
                lhs_reg = virtreg_to_realreg[inst->lhs->num];
                RegisterInfo *lhs_info = realreg[lhs_reg];
                if (lhs_info->endblock == block_index && lhs_info->endline == instruction_index) {
                    realreg[lhs_reg] = NULL;
                }
            }

            // rhs...
            if (inst->rhs && (inst->rhs->ty == V_REG || inst->rhs->ty == V_DEREF)) {
                rhs_reg = virtreg_to_realreg[inst->rhs->num];
                RegisterInfo *rhs_info = realreg[rhs_reg];
                // lhsと同じ可能性が存在する
                if (realreg[rhs_reg] && rhs_info->endblock == block_index && rhs_info->endline == instruction_index) {
                    realreg[rhs_reg] = NULL;
                }
            }

            // lvalがderefの時、そのレジスタは解放されるかもしれない
            if (inst->lval && (inst->lval->ty == V_DEREF)) {
                lval_reg = virtreg_to_realreg[inst->lval->num];
                RegisterInfo *lval_info = realreg[lval_reg];
                // lhs, rhsと
                if (realreg[lval_reg] && lval_info->endblock == block_index && lval_info->endline == instruction_index) {
                    realreg[lval_reg] = NULL;
                }
                continue;
            }

            if (inst->lval && lval_reg == ERR) {
                // spillはまだしないので
                if (lhs_reg > ERR && lhs_reg <= SPILL_RESERVED && realreg[lhs_reg] == NULL) {
                    lval_reg = lhs_reg;
                } else if (rhs_reg > ERR && rhs_reg <= SPILL_RESERVED && realreg[rhs_reg] == NULL) {
                    lval_reg = rhs_reg;
                } else {
                    for (int i = 1; i < REG_NUM; i++) {
                        if (realreg[i] == NULL) {
                            lval_reg = i;
                            break;
                        }
                    }
                    if (lval_reg == ERR) {
                        error("レジスタに割付できません");
                    }
                }
            }

            if (lval_reg) {
                RegisterInfo *info = reg_info->data[inst->lval->num];

                // 現状のΦ関数は最初のとこれで十分
                if (info->is_used_by_phi) {
                    if (phi_reg == ERR) {
                        phi_reg = lval_reg;
                        // phiを呼ばれる場所にもlval_regを書く
                        virtreg_to_realreg[info->phi_to] = lval_reg;
                        // realregにはphiの情報を書く
                        realreg[lval_reg] = reg_info->data[info->phi_to];
                    } else {
                        lval_reg = phi_reg;
                    }

                } else {
                    if (!(info->endblock == block_index && info->endline == instruction_index)) {
                        realreg[lval_reg] = reg_info->data[inst->lval->num];
                    }
                }
                virtreg_to_realreg[inst->lval->num] = lval_reg;
            }

            max_used_reg = MAX(lval_reg, max_used_reg);
        }
    }
    return max_used_reg;
}

void gen_function(Vec *funcs) {
    Function *f = at(funcs, 0);
    Vec *info = calc_reginfo(f);
    int *virtreg_to_realreg = malloc(sizeof(int) * info->len);
    int v = reg_assign(f, info, virtreg_to_realreg);
    for (int i = 0; i < info->len; i++) {
        printf("r%d: %d\n", i, virtreg_to_realreg[i]);
    }
    printf("max: %d\n", v);
}
