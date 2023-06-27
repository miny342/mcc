#include "mcc.h"

static Type int_type = {
    INT,
    NULL,
    0
};

static Type char_type = {
    CHAR,
    NULL,
    0
};

// エラー箇所を報告する
void error_at(char *loc, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    // locが含まれる行の開始地点と終了地点
    char *line = loc;
    while (user_input < line && line[-1] != '\n') {
        line--;
    }

    char *end = loc;
    while (*end != '\n') {
        end++;
    }

    // 見つかった行が全体の何番目か
    int line_num = 1;
    for (char *p = user_input; p < line; p++) {
        if (*p == '\n') {
            line_num++;
        }
    }

    // 見つかった行を、ファイル名と行番号と一緒に表示
    int ident = fprintf(stderr, "%s:%d ", filename, line_num);
    fprintf(stderr, "%.*s\n", (int)(end - line), line);

    // エラー個所を^で示して、エラーメッセージを表示
    int pos = loc - line + ident;
    fprintf(stderr, "%*s", pos, "");
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// 警告箇所を報告する
void warning_at(char *loc, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    // locが含まれる行の開始地点と終了地点
    char *line = loc;
    while (user_input < line && line[-1] != '\n') {
        line--;
    }

    char *end = loc;
    while (*end != '\n') {
        end++;
    }

    // 見つかった行が全体の何番目か
    int line_num = 1;
    for (char *p = user_input; p < line; p++) {
        if (*p == '\n') {
            line_num++;
        }
    }

    // 見つかった行を、ファイル名と行番号と一緒に表示
    int ident = fprintf(stderr, "%s:%d ", filename, line_num);
    fprintf(stderr, "%.*s\n", (int)(end - line), line);

    // エラー個所を^で示して、エラーメッセージを表示
    int pos = loc - line + ident;
    fprintf(stderr, "%*s", pos, "");
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
}

// エラーを報告するための関数
// printfと同じ引数を取る
void error(char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// 次のトークンが期待している記号の時には、トークンを一つ読み進めて
// 真を返す。それ以外の場合には偽を返す。
bool consume(char *op) {
    if (token->kind != TK_RESERVED || strlen(op) != token->len || memcmp(token->str, op, token->len))
        return false;
    token = token->next;
    return true;
}

// 次のトークンが変数の場合には、トークンを一つ読み進めて
// そのトークンを返す。それ以外の場合はNULLを返す
Token *consume_ident() {
    if (token->kind != TK_IDENT)
        return NULL;
    Token *tmp = token;
    token = token->next;
    return tmp;
}

// 次のトークンが...TKの場合にはトークンを一つ読み進めて
// 真を返す。それ以外は偽
bool consumeTK(NodeKind kind) {
    if (token->kind != kind)
        return false;
    token = token->next;
    return true;
}

// 次のトークンが期待している記号の時には、トークンを一つ読み進める。
// それ以外の場合にはエラーを報告する。
void expect(char *op) {
    if (token->kind != TK_RESERVED || strlen(op) != token->len || memcmp(token->str, op, token->len))
        error_at(token->str, "'%s'ではありません", op);
    token = token->next;
}

// 次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
int expect_number() {
    if (token->kind != TK_NUM) error_at(token->str, "数ではありません");
    int val = token->val;
    token = token->next;
    return val;
}

int is_alnum(char c) {
    return ('a' <= c && c <= 'z') ||
           ('A' <= c && c <= 'Z') ||
           ('0' <= c && c <= '9') ||
           (c == '_');
}

bool at_eof() {
    return token->kind == TK_EOF;
}

// 新しいトークンを作成してcurに繋げる
Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    tok->len = len;
    cur->next = tok;
    return tok;
}

// 入力文字列pをトークナイズしてそれを返す
Token *tokenize(char *p) {
    Token head;
    head.next = NULL;
    Token *cur = &head;
    int lvar_length;

    while(*p) {
        // 空白文字をスキップ
        if (isspace(*p)) {
            p++;
            continue;
        }

        if (strncmp(p, "continue", 8) == 0 && !is_alnum(p[8])) {
            cur = new_token(TK_CONTINUE, cur, p, 8);
            p += 8;
            continue;
        }

        if (strncmp(p, "return", 6) == 0 && !is_alnum(p[6])) {
            cur = new_token(TK_RETURN, cur, p, 6);
            p += 6;
            continue;
        }

        if (strncmp(p, "sizeof", 6) == 0 && !is_alnum(p[6])) {
            cur = new_token(TK_SIZEOF, cur, p, 6);
            p += 6;
            continue;
        }

        if (strncmp(p, "while", 5) == 0 && !is_alnum(p[5])) {
            cur = new_token(TK_WHILE, cur, p, 5);
            p += 5;
            continue;
        }

        if (strncmp(p, "break", 5) == 0 && !is_alnum(p[5])) {
            cur = new_token(TK_BREAK, cur, p, 5);
            p += 5;
            continue;
        }

        if (strncmp(p, "else", 4) == 0 && !is_alnum(p[4])) {
            cur = new_token(TK_ELSE, cur, p, 4);
            p += 4;
            continue;
        }

        if (strncmp(p, "char", 4) == 0 && !is_alnum(p[4])) {
            cur = new_token(TK_CHAR, cur, p, 4);
            p += 4;
            continue;
        }

        if (strncmp(p, "void", 4) == 0 && !is_alnum(p[4])) {
            cur = new_token(TK_VOID, cur, p, 4);
            p += 4;
            continue;
        }

        if (strncmp(p, "int", 3) == 0 && !is_alnum(p[3])) {
            cur = new_token(TK_INT, cur, p, 3);
            p += 3;
            continue;
        }

        if (strncmp(p, "for", 3) == 0 && !is_alnum(p[3])) {
            cur = new_token(TK_FOR, cur, p, 3);
            p += 3;
            continue;
        }

        if (strncmp(p, "...", 3) == 0 || strncmp(p, "<<=", 3) == 0 ||
            strncmp(p, ">>=", 3) == 0) {
                cur = new_token(TK_RESERVED, cur, p, 3);
                p += 3;
                continue;
        }

        if (strncmp(p, "if", 2) == 0 && !is_alnum(p[2])) {
            cur = new_token(TK_IF, cur, p, 2);
            p += 2;
            continue;
        }

        if (strncmp(p, "//", 2) == 0) {
            while(*p && *p != '\n') {
                p++;
                continue;
            }
            continue;
        }

        if (strncmp(p, "/*", 2) == 0) {
            char *tmp = strstr(p, "*/");
            if (!tmp) error_at(p, "*/ が見つかりません");
            p = tmp + 2;
            continue;
        }

        if (*p == '\"') {
            char *tmp = strstr(p + 1, "\"");
            if (!tmp) error_at(p, "\" が見つかりません");
            tmp++;
            cur = new_token(TK_STR, cur, p, tmp - p);
            p = tmp;
            continue;
        }

        if (strncmp(p, "==", 2) == 0 || strncmp(p, "!=", 2) == 0 ||
            strncmp(p, "<=", 2) == 0 || strncmp(p, ">=", 2) == 0 ||
            strncmp(p, "<<", 2) == 0 || strncmp(p, ">>", 2) == 0 ||
            strncmp(p, "&&", 2) == 0 || strncmp(p, "||", 2) == 0 ||
            strncmp(p, "++", 2) == 0 || strncmp(p, "--", 2) == 0 ||
            strncmp(p, "+=", 2) == 0 || strncmp(p, "-=", 2) == 0 ||
            strncmp(p, "*=", 2) == 0 || strncmp(p, "/=", 2) == 0 ||
            strncmp(p, "%=", 2) == 0 || strncmp(p, "&=", 2) == 0 ||
            strncmp(p, "|=", 2) == 0 || strncmp(p, "^=", 2) == 0) {
                cur = new_token(TK_RESERVED, cur, p, 2);
                p += 2;
                continue;
            }

        if (isalpha(*p) || *p == '_') {
            lvar_length = 0;
            while(is_alnum(p[lvar_length]))
                lvar_length++;
            cur = new_token(TK_IDENT, cur, p, lvar_length);
            p += lvar_length;
            continue;
        }

        if (strchr("+-*/()<>;={},&[]%^|?:~!", *p)) {
            cur = new_token(TK_RESERVED, cur, p++, 1);
            continue;
        }

        if (isdigit(*p)) {
            cur = new_token(TK_NUM, cur, p, 0);
            cur->val = strtol(p, &p, 10);
            continue;
        }

        error_at(p, "トークナイズできません");
    }

    new_token(TK_EOF, cur, p, 0);
    return head.next;
}


// 変数を名前で検索。なければNULL
LVar *find_lvar(Token *tok) {
    for (LVar *var = locals; var; var = var->next)
      if (var->len == tok->len && !memcmp(tok->str, var->name, var->len))
        return var;
    return NULL;
}

GVar *find_gvar(Token *tok) {
    for (GVar *var = code; var; var = var->next) {
        if (var->len == tok->len && !memcmp(tok->str, var->name, var->len)) {
            return var;
        }
    }
    return NULL;
}

// 事前計算が単純な範囲で計算を行う
Node *calc_node(Node *node) {
    if (!node)
        return NULL;

    node->lhs = calc_node(node->lhs);
    node->rhs = calc_node(node->rhs);

    if (!node->lhs || node->lhs->kind != ND_NUM) {
        return node;
    }

    int l = node->lhs->val;
    switch (node->kind) {
        case ND_NEG:
            return new_node_num(-l);
        case ND_NOT:
            return new_node_num(!l);
        case ND_BITNOT:
            return new_node_num(~l);
    }

    if (!node->rhs || node->rhs->kind != ND_NUM) {
        return node;
    }

    int r = node->rhs->val;
    int res;

    switch (node->kind) {
        case ND_ADD:
            res = l + r;
            break;
        case ND_SUB:
            res = l - r;
            break;
        case ND_MUL:
            res = l * r;
            break;
        case ND_DIV:
            res = l / r;
            break;
        case ND_REMINDER:
            res = l % r;
            break;
        case ND_AND:
            res = l && r;
            break;
        case ND_OR:
            res = l || r;
            break;
        case ND_LSHIFT:
            res = l << r;
            break;
        case ND_RSHIFT:
            res = l >> r;
            break;
        case ND_BITAND:
            res = l & r;
            break;
        case ND_BITOR:
            res = l | r;
            break;
        case ND_BITXOR:
            res = l ^ r;
            break;
        case ND_EQ:
            res = l == r;
            break;
        case ND_NE:
            res = l != r;
            break;
        case ND_LT:
            res = l < r;
            break;
        case ND_LE:
            res = l <= r;
            break;
        default:
            return node;
    }
    return new_node_num(res);
}

int calc(Node *node) {
    Node *n = calc_node(node);
    if (n->kind != ND_NUM) {
        error_at(token->str, "calc error");
    }
    return n->val;
}

int sizeof_parse(Type *type) {
    if (!type)
        error_at(token->str, "type parse error");
    else if (type->ty == INT)
        return sizeof(int);
    else if (type->ty == PTR)
        return sizeof(int*);
    else if (type->ty == ARRAY)
        return type->array_size * sizeof_parse(type->ptr_to);
    else if (type->ty == CHAR || type->ty == VOID || type->ty == FUNC)
        return sizeof(char);
    else error_at(token->str, "不明な型のsizeof");
}

int get_align(Type *type) {
    if (!type)
        error_at(token->str, "type parse error");
    else if (type->ty == ARRAY)
        return get_align(type->ptr_to);
    else sizeof_parse(type);
}

// 不完全な型を返す。bottomに、不完全な末端があるので埋めること
Type *eval_type_acc(Token **ident, Type **bottom) {

    // ポインタが一つ以上の時、topが先頭、btmが末端になるよう読む
    Type *top = NULL;
    Type *btm = NULL;
    while(consume("*")) {
        if (top == NULL) {  // 初回
            top = calloc(1, sizeof(Type));
            btm = top;
        } else {
            btm->ptr_to = calloc(1, sizeof(Type));
            btm = btm->ptr_to;
        }
        btm->ty = PTR;
    }

    // ()で囲まれていると、その中の型計算を優先する
    Type *tmp_top = NULL;
    Type *tmp_btm = NULL;
    if (consume("(")) {
        tmp_top = eval_type_acc(ident, &tmp_btm);
        expect(")");
    }

    // identはアドレスに入れる
    if (ident && !*ident) {
        *ident = consume_ident();
    } else {
        Token *tok = consume_ident();
        if (tok) {
            error_at(tok->str, "トークンが存在します(コンパイラのバグ)");
        }
    }

    // ()なら関数、[]なら配列 []は並べることができる
    Type *top_ = NULL;
    Type *btm_ = NULL;
    while(1) {
        if (consume("(")) {
            if (top_ == NULL) {
                top_ = calloc(1, sizeof(Type));
                btm_ = top_;
            } else {
                if (top_->ty == FUNC) {
                    error_at(token->str, "関数を返す関数は利用できません");
                } else if (top_->ty == ARRAY) {
                    error_at(token->str, "関数の配列は利用できません");
                } else {
                    error_at(token->str, "unexpected error func");
                }
            }
            btm_->ty = FUNC;
            Type **args = &btm_->args;
            if (!consume(")")) {
                // 引数を読む
                while(1) {
                    *args = eval_type_all();
                    btm_->arglen += 1;
                    if ((*args)->ty == ARRAY) (*args)->ty = PTR; // 関数の引数ではint a[10]などはすべてポインタとする
                    if ((*args)->ty == VA_ARGS || !consume(",")) break;
                    args = &(*args)->args;
                }
                expect(")");
            }
        } else if (consume("[")) {
            if (top_ == NULL) {
                top_ = calloc(1, sizeof(Type));
                btm_ = top_;
            } else if (btm_->ty != FUNC) {
                btm_->ptr_to = calloc(1, sizeof(Type));
                btm_ = btm_->ptr_to;
            } else {
                error_at(token->str, "配列を返す関数は利用できません");
            }
            btm_->ty = ARRAY;
            if (consume("]")) {
                btm_->array_size = -1; // 長さが不定の場合は-1
            } else {
                btm_->array_size = calc(logic_or());
                expect("]");
            }
        } else {
            break;
        }
    }

    // top, btmに上記の型情報をまとめる

    // top_, btm_とtop, btmを結合
    if (top_) {
        if (top == NULL) {
            btm = btm_;
        }
        if (btm_->ty == FUNC) {
            btm_->ret = top;
        } else if (btm_->ty == ARRAY) {
            btm_->ptr_to = top;
        } else {
            error_at(token->str, "unexpected error top_");
        }
        top = top_;
    }

    // tmp_top, tmp_btmとtop, btmを結合
    if (tmp_top) {
        if (top == NULL) {
            btm = tmp_btm;
        } else if (tmp_btm->ty == FUNC) {
            if (top->ty == ARRAY) {
                error_at(token->str, "配列を返す関数は利用できません");
            } else if (top->ty == FUNC) {
                error_at(token->str, "関数を返す関数は利用できません");
            }
            tmp_btm->ret = top;
        } else if (tmp_btm->ty == ARRAY) {
            if (top->ty == FUNC) {
                error_at(token->str, "関数の配列は利用できません");
            }
            tmp_btm->ptr_to = top;
        } else if (tmp_btm->ty == PTR) {
            tmp_btm->ptr_to = top;
        } else {
            error_at(token->str, "unexpected error tmp_top");
        }
        top = tmp_top;
    }

    // btm == NULLかつtop == NULLか、btm != NULLかつtop != NULLしかとらない(はず)
    *bottom = btm;
    return top;
}

// 完全な型を返す
Type *eval_type_all() {
    Type *type = calloc(1, sizeof(Type));
    if(consumeTK(TK_INT)) {
        type->ty = INT;
    } else if (consumeTK(TK_CHAR)) {
        type->ty = CHAR;
    } else if (consume("...")) {
        type->ty = VA_ARGS;
    } else if (consumeTK(TK_VOID)) {
        type->ty = VOID;
    } else {
        error_at(token->str, "no int or char");
    }
    Token *tok = NULL;
    Type *btm = NULL;
    Type *top = eval_type_acc(&tok, &btm);
    if (btm) {
        if (btm->ty == FUNC) {
            btm->ret = type;
        } else if (btm->ty == ARRAY || btm->ty == PTR) {
            btm->ptr_to = type;
        } else {
            error_at(token->str, "unexpected error all btm, %d\n", btm->ty);
        }
    } else {
        type->tok = tok;
        return type;
    }
    top->tok = tok;
    return top;
}

int calc_aligned(int offset, Type *type) {
    offset += sizeof_parse(type);
    int align = get_align(type);
    if (offset % align) {
        offset += (align - offset % align);
    }
    return offset;
}

LVar *init_lvar(Type *type) {
    if (type->ty == VA_ARGS) {
        error_at(token->str, "assign lvarにva argsが入力されました(unimplemented)");
    }
    if (find_lvar(type->tok)) {
        error_at(type->tok->str, "duplicate");
    }

    LVar *lvar = calloc(1, sizeof(LVar));
    lvar->next = locals;
    lvar->name = type->tok->str;
    lvar->len = type->tok->len;
    lvar->offset = calc_aligned((locals ? locals->offset : 0), type);
    lvar->type = type;
    return lvar;
}

void assign_lvar_arr(LVar *lvar, int len) {
    lvar->type->array_size = len;
    lvar->offset = calc_aligned((locals ? locals->offset : 0), lvar->type);
}

GVar *init_gvar(Type *type, Token *tok) {
    if (find_gvar(tok)) {
        error_at(token->str, "duplicate");
    }
    GVar *gvar = calloc(1, sizeof(GVar));
    gvar->name = tok->str;
    gvar->len = tok->len;
    gvar->type = type;
    gvar->size = sizeof_parse(type);
    return gvar;
}

Node *lvar_node(LVar *lvar) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_LVAR;
    node->lvar = lvar;
    if (lvar->type->ty == ARRAY) {
        node->type = calloc(1, sizeof(Type));
        node->type->ty = PTR;
        node->type->ptr_to = lvar->type->ptr_to;
        Node *res = new_node(ND_ADDR, node, NULL, node->type);
        res->lvar = node->lvar;
        return res;
    } else {
        node->type = lvar->type;
    }
    return node;
}

Node *new_node(NodeKind kind, Node *lhs, Node *rhs, Type *type) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    node->lhs = lhs;
    node->rhs = rhs;
    node->type = type;
    return node;
}

Node *new_node_num(int val) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_NUM;
    node->val = val;
    node->type = &int_type;
    return node;
}

Node *add_node(Node *lhs, Node *rhs) {
    if (lhs->type->ty == PTR && rhs->type->ty == PTR) {
        error_at(token->str, "ptr + ptrはできません");
    }
    if (lhs->type->ty == PTR) {
        return new_node(ND_ADD, lhs, new_node(ND_MUL, rhs, new_node_num(sizeof_parse(lhs->type->ptr_to)), NULL), lhs->type);
    }
    if (rhs->type->ty == PTR) {
        return new_node(ND_ADD, rhs, new_node(ND_MUL, lhs, new_node_num(sizeof_parse(rhs->type->ptr_to)), NULL), rhs->type);
    }
    return new_node(ND_ADD, lhs, rhs, lhs->type);
}

Node *sub_node(Node *lhs, Node *rhs) {
    if(lhs->type->ty == PTR && rhs->type->ty == PTR) {
        return new_node(ND_SUB, lhs, rhs, &int_type);
    }
    if(lhs->type->ty == PTR) {
        return new_node(ND_SUB, lhs, new_node(ND_MUL, new_node_num(sizeof_parse(lhs->type->ptr_to)), rhs, NULL), lhs->type);
    }
    if(rhs->type->ty == PTR) {
        error_at(token->str, "int - ptrは未定義です");
    }
    return new_node(ND_SUB, lhs, rhs, &int_type);
}

Node *deref_node(Node *n) {
    if (n->type->ty != PTR) {
        error_at(token->str, "derefできません");
    }
    return new_node(ND_DEREF, n, NULL, n->type->ptr_to);
}

Node *assign_node(Node *lhs, Node *rhs) {
    return new_node(ND_ASSIGN, lhs, rhs, lhs->type);
}

Node *block_node(Node *child, Node *prev) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_BLOCK;
    node->lhs = child;
    node->rhs = prev;
    return node;
}

char *type_name_arr[] = {
    "int", "ptr", "array", "char", "func", "...", "void",
};

void show_type(Type *type, int indent) {
    if (type == NULL) {
        fprintf(stderr, "%*sNULL\n", indent, "");
        return;
    }
    if(type->tok) {
        fprintf(stderr, "%*sname: %.*s\n", indent, "", type->tok->len, type->tok->str);
    }
    fprintf(stderr, "%*sType: %s\n", indent, "", type_name_arr[type->ty]);
    if (type->ty == PTR) {
        show_type(type->ptr_to, indent + 2);
    } else if (type->ty == ARRAY) {
        fprintf(stderr, "%*slen: %d\n", indent + 2, "", type->array_size);
        show_type(type->ptr_to, indent + 2);
    } else if (type->ty == FUNC) {
        fprintf(stderr, "%*sargs:\n", indent + 2, "");
        Type *t = type->args;
        for(; t; t = t->args) {
            show_type(t, indent + 4);
        }
        fprintf(stderr, "%*sreturn:\n", indent + 2, "");
        show_type(type->ret, indent + 4);
    }
}

void type_test() {
    while(!at_eof()) {
        Token *ident = NULL;
        Type *type = eval_type_all(&ident);
        show_type(type, 0);
        if (consume("{")) {
            int i = 1;
            while (1) {
                if (consume("{")) i++;
                else if (consume("}")) i--;
                else token = token->next;
                if (i == 0) break;
            }
        } else if (!consume(";")) {
            error_at(token->str, "type parse error");
        }
    }

    exit(1);
}

void program() {
    GVar **tmp = &code;
    globalstmt(tmp);
    while(!at_eof()) {
        tmp = &(*tmp)->next;
        globalstmt(tmp);
    }
}

void globalstmt(GVar **ptr) {
    Type *type = eval_type_all();
    Token *tok = type->tok;
    if (!type || !tok) error_at(token->str, "型が正しくありません");
    GVar *gvar = init_gvar(type, tok);
    if (type->ty == FUNC) {
        if (consume(";")) {
            *ptr = gvar;
            return;
        }
        locals = NULL;

        Type *args = type->args;
        while (args) {
            if (args->ty == VA_ARGS) {
                break;
            }
            LVar *lvar = init_lvar(args);
            locals = lvar;
            args = args->args;
        }
        LVar *tmp = locals;
        LVar *lvar = locals;
        for(int i = type->arglen; i > 6; i--) {
            if (i > 6) {
                lvar->offset = (5 - i) * 8;
            }
            if (i == 7) {
                locals = lvar->next;
                lvar->next = NULL;
                for(lvar = locals; lvar->next; lvar = lvar->next);
                lvar->next = tmp;
                break;
            }
            lvar = lvar->next;
        }
        *ptr = gvar;
        gvar->node = calc_node(stmt());
        gvar->locals = locals;
        return;
    } else {
        GVar *gvar = init_gvar(type, tok);
        if (consume("=")) {
            if (gvar->type->ty == ARRAY) {
                int i = 0;
                Node *node;
                Node *now;

                if (consume("{")) {
                    if (!consume("}")) {
                        node = calloc(1, sizeof(Node));
                        now = assign();
                        node->lhs = now;
                        node->rhs = calloc(1, sizeof(Node));
                        node->kind = ND_BLOCK;
                        node->rhs->kind = ND_BLOCK;
                        now = node->rhs;
                        i++;
                        while (consume(",")) {
                            now->lhs = assign();
                            now->rhs = calloc(1, sizeof(Node));
                            now->rhs->kind = ND_BLOCK;
                            now = now->rhs;
                            i++;
                        }
                        expect("}");
                    }
                    if (type->array_size != -1) {
                        if (i > type->array_size) {
                            error_at(token->str, "引数が多すぎます");
                        }
                    } else {
                        type->array_size = i;
                        gvar->size = sizeof_parse(type);
                    }
                    gvar->node = node;
                } else if (token->kind == TK_STR) {
                    int i = 0;
                    char *p = token->str + 1;
                    node = calloc(1, sizeof(Node));
                    node->kind = ND_BLOCK;
                    Node *now = node;
                    for(; i < token->len - 2; i++) {
                        now->lhs = calloc(1, sizeof(Node));
                        now->lhs->kind = ND_NUM;
                        now->lhs->type = &char_type;
                        now->lhs->val = p[i];
                        now->rhs = calloc(1, sizeof(Node));
                        now->rhs->kind = ND_BLOCK;
                        now = now->rhs;
                    }
                    now->lhs = calloc(1, sizeof(Node));
                    now->lhs->kind = ND_NUM;
                    now->lhs->type = &char_type;
                    i++;
                    if (type->array_size != -1) {
                        if (i > type->array_size) {
                            error_at(token->str, "文字列が長すぎます");
                        }
                    } else {
                        type->array_size = i;
                        gvar->size = sizeof_parse(type);
                    }
                    gvar->node = node;
                    token = token->next;
                } else {
                    error_at(token->str, "unimplemented");
                }
            } else {
                gvar->node = calloc(1, sizeof(Node));
                gvar->node->lhs = assign();
                gvar->node->kind = ND_BLOCK;
            }
        }
        if (type->array_size == -1) {
            error_at(token->str, "配列の初期化ができません");
        }
        expect(";");
        gvar->node = calc_node(gvar->node);
        *ptr = gvar;
    }
}

Node *stmt() {
    Node *node;
    if(consumeTK(TK_RETURN)) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_RETURN;
        node->lhs = expr();
        expect(";");
    } else if (consumeTK(TK_IF)) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_IF;
        expect("(");
        node->lhs = expr();
        expect(")");
        node->rhs = stmt();
        if (consumeTK(TK_ELSE))
            node->rhs = new_node(ND_ELSE, node->rhs, stmt(), NULL);
    } else if (consumeTK(TK_WHILE)) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_WHILE;
        expect("(");
        node->lhs = expr();
        expect(")");
        node->rhs = stmt();
    } else if (consumeTK(TK_FOR)) {
        node = new_node(ND_FOR, NULL, new_node(ND_FOR, NULL, new_node(ND_FOR, NULL, NULL, NULL), NULL), NULL);
        expect("(");
        if(!consume(";")) {
            node->lhs = expr();
            expect(";");
        }
        if(!consume(";")) {
            node->rhs->lhs = expr();
            expect(";");
        } else {
            node->rhs->lhs = new_node_num(1);
        }
        if(!consume(")")) {
            node->rhs->rhs->lhs = expr();
            expect(")");
        }
        node->rhs->rhs->rhs = stmt();
    } else if (consume("{")) {
        node = block_node(NULL, NULL);
        while(!consume("}")) {
            node = block_node(stmt(), node);
        }
    } else if(consumeTK(TK_BREAK)) {
        node = new_node(ND_BREAK, NULL, NULL, NULL);
        expect(";");
    } else if(consumeTK(TK_CONTINUE)) {
        node = new_node(ND_CONTINUE, NULL, NULL, NULL);
        expect(";");
    } else {
        node = expr();
        expect(";");
    }
    return node;
}

// (int | char) indent ([number])? (= assign | )? | assign
Node *expr() {
    if(token->kind == TK_INT || token->kind == TK_CHAR) {
        Type *type = eval_type_all();
        LVar *lvar = init_lvar(type);
        Node *node = NULL;
        if (consume("=")) {
            if (lvar->type->ty == ARRAY) {
                Node *ptr_node = calloc(1, sizeof(Node));
                ptr_node->kind = ND_LVAR;
                ptr_node->lvar = lvar;

                ptr_node->type = calloc(1, sizeof(Type));
                ptr_node->type->ty = PTR;
                ptr_node->type->ptr_to = lvar->type->ptr_to;

                ptr_node = new_node(ND_ADDR, ptr_node, NULL, ptr_node->type);
                ptr_node->lvar = lvar;

                int i = 0;
                if (consume("{")) {
                    if (!consume("}")) {
                        while(1) {
                            node = block_node(assign_node(deref_node(add_node(ptr_node, new_node_num(i++))), assign()), node);
                            if (consume("}")) break;
                            expect(",");
                        }
                        if (lvar->type->array_size != -1) {
                            for(; i < lvar->type->array_size; i++) {
                                node = block_node(assign_node(deref_node(add_node(ptr_node, new_node_num(i))), new_node_num(0)), node);
                            }
                            if (i > lvar->type->array_size) {
                                error_at(token->str, "配列が長すぎます");
                            }
                        } else {
                            assign_lvar_arr(lvar, i);
                        }
                    } else {
                        if (lvar->type->array_size != -1) {
                            for(i = 0; i < lvar->type->array_size; i++) {
                                node = block_node(assign_node(deref_node(add_node(ptr_node, new_node_num(i))), new_node_num(0)), node);
                            }
                        } else {
                            assign_lvar_arr(lvar, 0);
                        }
                    }
                } else if (token->kind == TK_STR) {
                    char *p = token->str + 1;
                    while (*p != '"') {
                        node = block_node(assign_node(deref_node(add_node(ptr_node, new_node_num(i++))), new_node_num(*(p++))), node);
                    }
                    node = block_node(assign_node(deref_node(add_node(ptr_node, new_node_num(i++))), new_node_num(0)), node);
                    if (lvar->type->array_size != -1) {
                        for (; i < lvar->type->array_size; i++) {
                            node = block_node(assign_node(deref_node(add_node(ptr_node, new_node_num(i))), new_node_num(0)), node);
                        }
                        if (i > lvar->type->array_size) {
                            error_at(p, "文字列が長すぎます");
                        }
                    } else {
                        assign_lvar_arr(lvar, i);
                    }
                    token = token->next;
                } else {
                    error_at(token->str, "unexpected token");
                }
            } else {
                node = new_node(ND_ASSIGN, lvar_node(lvar), assign(), lvar->type);
            }
        }
        if (type->array_size == -1) {
            error_at(token->str, "配列の初期化ができません");
        }
        locals = lvar;
        return new_node(ND_DECLARATION, node, NULL, NULL);
    }
    return assign();
}

// logic_or ( = assign )*
Node *assign() {
    Node *node = if_op();

    if(consume("=")) {
        Node *right = assign();
        return assign_node(node, right);
    }
    if(consume("+=")) {
        Node *right = assign();
        return assign_node(node, add_node(node, right));
    }
    if(consume("-=")) {
        Node *right = assign();
        return assign_node(node, sub_node(node, right));
    }
    if(consume("*=")) {
        Node *right = assign();
        if (node->type->ty == PTR || right->type->ty == PTR) {
            error_at(token->str, "ポインタの掛け算は未定義です");
        }
        return assign_node(node, new_node(ND_MUL, node, right, &int_type));
    }
    if(consume("/=")) {
        Node *right = assign();
        if (node->type->ty == PTR || right->type->ty == PTR) {
            error_at(token->str, "ポインタの割り算は未定義です");
        }
        return assign_node(node, new_node(ND_DIV, node, right, &int_type));
    }
    if(consume("%=")) {
        Node *right = assign();
        if (node->type->ty == PTR || right->type->ty == PTR) {
            error_at(token->str, "ポインタの余剰は未定義です");
        }
        return assign_node(node, new_node(ND_REMINDER, node, right, &int_type));
    }
    if(consume("<<=")) {
        Node *right = assign();
        if (node->type->ty == PTR || right->type->ty == PTR) {
            error_at(token->str, "ポインタのシフト計算は未定義です");
        }
        return assign_node(node, new_node(ND_LSHIFT, node, right, &int_type));
    }
    if(consume(">>=")) {
        Node *right = assign();
        if (node->type->ty == PTR || right->type->ty == PTR) {
            error_at(token->str, "ポインタのシフト計算は未定義です");
        }
        return assign_node(node, new_node(ND_RSHIFT, node, right, &int_type));
    }
    if(consume("&=")) {
        Node *right = assign();
        if (node->type->ty == PTR || right->type->ty == PTR) {
            error_at(token->str, "ポインタのAND計算は未定義です");
        }
        return assign_node(node, new_node(ND_BITAND, node, right, &int_type));
    }
    if(consume("|=")) {
        Node *right = assign();
        if (node->type->ty == PTR || right->type->ty == PTR) {
            error_at(token->str, "ポインタのOR計算は未定義です");
        }
        return assign_node(node, new_node(ND_BITOR, node, right, &int_type));
    }
    if(consume("^=")) {
        Node *right = assign();
        if (node->type->ty == PTR || right->type->ty == PTR) {
            error_at(token->str, "ポインタのXOR計算は未定義です");
        }
        return assign_node(node, new_node(ND_BITXOR, node, right, &int_type));
    }

    return node;
}

// logic_or (? assign : logic_or)?
Node *if_op() {
    Node *node = logic_or();

    if(consume("?")) {
        Node *middle = assign(); // ?:の間はかっこで囲まれている扱い
        expect(":");
        Node *last = logic_or();
        node = new_node(ND_IF, node, new_node(ND_ELSE, middle, last, NULL), middle->type);
    }
    return node;
}

// logic_and ( || logic_and )*
Node *logic_or() {
    Node *node = logic_and();

    for(;;) {
        if (consume("||"))
            node = new_node(ND_OR, node, logic_and(), node->type);
        else
            return node;
    }
}

// bit_or ( && bit_or )*
Node *logic_and() {
    Node *node = bit_or();

    for(;;) {
        if (consume("&&"))
            node = new_node(ND_AND, node, bit_or(), node->type);
        else
            return node;
    }
}

// bit_xor ( | bit_xor )*
Node *bit_or() {
    Node *node = bit_xor();

    for(;;) {
        if (consume("|"))
            node = new_node(ND_BITOR, node, bit_xor(), node->type);
        else
            return node;
    }
}

// bit_and ( ^ bit_and )*
Node *bit_xor() {
    Node *node = bit_and();

    for(;;) {
        if (consume("^"))
            node = new_node(ND_BITXOR, node, bit_and(), node->type);
        else
            return node;
    }
}

// equality ( & equality )*
Node *bit_and() {
    Node *node = equality();

    for(;;) {
        if (consume("&"))
            node = new_node(ND_BITAND, node, equality(), node->type);
        else
            return node;
    }
}

// relational ("==" relational | "!=" relational)*
Node *equality() {
    Node *node = relational();

    for(;;){
        if (consume("=="))
            node = new_node(ND_EQ, node, relational(), &int_type);
        else if (consume("!="))
            node = new_node(ND_NE, node, relational(), &int_type);
        else
            return node;
    }
}

// shift ("<" shift | "<=" shift | ">" shift | ">=" shift)*
Node *relational() {
    Node *node = shift();

    for(;;) {
        if (consume("<="))
            node = new_node(ND_LE, node, shift(), &int_type);
        else if (consume(">="))
            node = new_node(ND_LE, shift(), node, &int_type);
        else if (consume("<"))
            node = new_node(ND_LT, node, shift(), &int_type);
        else if (consume(">"))
            node = new_node(ND_LT, shift(), node, &int_type);
        else
            return node;
    }
}

// add ( << add | >> add )*
Node *shift() {
    Node *node = add();

    for(;;) {
        if (consume("<<")) {
            node = new_node(ND_LSHIFT, node, add(), node->type);
        } else if (consume(">>")) {
            node = new_node(ND_RSHIFT, node, add(), node->type);
        }
        else
            return node;
    }
}

// mul ( + mul | - mul )*
Node *add() {
    Node *node = mul();
    Node *right;

    for(;;) {
        if (consume("+")) {
            right = mul();
            node = add_node(node ,right);
        }
        else if (consume("-")) {
            right = mul();
            node = sub_node(node, right);
        }
        else
            return node;
    }
}

// unary ( * unary | / unary | % unary )*
Node *mul() {
    Node *node = unary();
    Node *right;

    for(;;) {
        if (consume("*")) {
            right = unary();
            if (node->type->ty == PTR || right->type->ty == PTR)
                error_at(token->str, "ptr * ptr is not defined");
            node = new_node(ND_MUL, node, right, &int_type);
        }
        else if (consume("/")) {
            right = unary();
            if (node->type->ty == PTR || right->type->ty == PTR)
                error_at(token->str, "ptr / ptr is not defined");
            node = new_node(ND_DIV, node, right, &int_type);
        }
        else if (consume("%")) {
            right = unary();
            if (node->type->ty == PTR || right->type->ty == PTR)
                error_at(token->str, "ptr %% ptr is not defined");
            node = new_node(ND_REMINDER, node, right, &int_type);
        }
        else
            return node;
    }
}

// primary (("[" assign "]")* | "(" assign,* ")" | ++ | -- )  |  ("*" | "&" | + | - | ! | ~) unary  |  sizeof unary
Node *unary() {
    Node *node;
    Type *tmp;
    if (consume("+"))
        return unary();
    if (consume("-")) {
        node = unary();
        if (node->type->ty == PTR)
            error_at(token->str, "-ptr is not defined");
        return new_node(ND_NEG, node, NULL, node->type);
    }
    if (consume("*")) {
        if (consume("&")) return unary();
        node = unary();
        if (node->type->ty != PTR)
            error_at(token->str, "this is not ptr");
        return new_node(ND_DEREF, node, NULL, node->type->ptr_to);
    }
    if (consume("&")) {
        if (consume("&")) error_at(token->str, "&& is not usable");
        if (consume("*")) return unary();
        node = unary();
        if (node->lvar != NULL && node->lvar->type->ty == ARRAY) {
            node->lvar = NULL;
            return node;
        }
        tmp = calloc(1, sizeof(Type));
        tmp->ptr_to = node->type;
        tmp->ty = PTR;
        return new_node(ND_ADDR, node, NULL, tmp);
    }
    if (consumeTK(TK_SIZEOF)) {
        node = unary();
        if (node->lvar == NULL) {
            return new_node_num(sizeof_parse(node->type));
        } else {
            return new_node_num(sizeof_parse(node->lvar->type));
        }
    }
    if (consume("++")) {
        node = unary();
        return new_node(ND_INCR, node, NULL, node->type);
    }
    if (consume("--")) {
        node = unary();
        return new_node(ND_DECR, node, NULL, node->type);
    }
    if (consume("!")) {
        node = unary();
        return new_node(ND_NOT, node, NULL, node->type);
    }
    if (consume("~")) {
        node = unary();
        return new_node(ND_BITNOT, node, NULL, node->type); // TODO: charをintとして扱う
    }
    node = primary();
    Node *right;
    for(;;) {
        if (consume("[")) {
            Node *n = assign();
            node = deref_node(add_node(node, n));
            expect("]");
        } else if (consume("(")) {
            if (node->type->ty != FUNC && !(node->type->ty == PTR && node->type->ptr_to->ty == FUNC)) {
                error_at(token->str, "関数ではありません");
            }
            Node *n = calloc(1, sizeof(Node));
            n->kind = ND_CALL;
            if (node->type->ty == PTR) {
                n->type = node->type->ptr_to->ret;
            } else {
                n->type = node->type->ret;
            }
            n->lhs = node;
            Node *now_node = calloc(1, sizeof(Node));
            n->rhs = now_node;

            if (!consume(")")) {
                for(;;) {
                    now_node->lhs = assign();
                    now_node->rhs = calloc(1, sizeof(Node));
                    now_node = now_node->rhs;
                    if(!consume(",")) break;
                }
                expect(")");
            }
            node = n;
        } else if (consume("++")) {
            node = new_node(ND_INCR, NULL, node, node->type);
        } else if (consume("--")) {
            node = new_node(ND_DECR, NULL, node, node->type);
        } else
            return node;
    }
}

// num | "(" assign ")" | ident | String
Node *primary() {
    if (consume("(")){
        Node *node = assign();
        expect(")");
        return node;
    }

    if (token->kind == TK_STR) {
        String *s = strmapget(strmap, token->str, token->len);
        if (!s) {
            s = calloc(1, sizeof(String));
            s->next = strs;
            s->offset = strs->offset + 1;
            s->tok = token;
            strmapset(strmap, token->str, token->len, s);
            strs = s;
        }
        token = token->next;

        Node *node = calloc(1, sizeof(Node));
        node->kind = ND_STR;
        node->s = s;
        node->type = calloc(1, sizeof(Type));
        node->type->ty = PTR;
        node->type->ptr_to = &char_type;
        return node;
    }

    Token *tok = consume_ident();
    if(tok) {
        LVar *lvar = find_lvar(tok);
        if (lvar) {
            return lvar_node(lvar);
        } else {
            GVar *gvar = find_gvar(tok);
            if (gvar) {
                Node *node = calloc(1, sizeof(Node));
                node->kind = ND_GLOVAL_LVAR;
                node->gvar = gvar;
                if (gvar->type->ty == ARRAY) {
                    node->type = calloc(1, sizeof(Type));
                    node->type->ty = PTR;
                    node->type->ptr_to = gvar->type->ptr_to;
                    Node *res = new_node(ND_ADDR, node, NULL, node->type);
                    res->gvar = node->gvar;
                    return res;
                } else {
                    node->type = gvar->type;
                }
                return node;
            }
        }
        error_at(tok->str, "undefined");
    }

    return new_node_num(expect_number());
}