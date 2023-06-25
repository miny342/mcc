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

        if (strncmp(p, "...", 3) == 0 && !is_alnum(p[3])) {
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
            strncmp(p, "&&", 2) == 0 || strncmp(p, "||", 2) == 0) {
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

        if (strchr("+-*/()<>;={},&[]%^|", *p)) {
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

Type *eval_type() {
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
    return type;
}

// 事前計算が単純な範囲で計算を行う
Node *calc_node(Node *node) {
    if (!node)
        return NULL;

    node->lhs = calc_node(node->lhs);
    node->rhs = calc_node(node->rhs);

    if (!node->lhs || !node->rhs) {
        return node;
    }

    if (node->lhs->kind != ND_NUM || node->rhs->kind != ND_NUM) {
        return node;
    }

    int l = node->lhs->val;
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
    if (type->ty == INT)
        return sizeof(int);
    if (type->ty == PTR)
        return sizeof(int*);
    if (type->ty == ARRAY)
        return type->array_size * sizeof_parse(type->ptr_to);
    if (type->ty == CHAR || type->ty == VOID)
        return sizeof(char);
}

LVar *assign_lvar(Type *type) {
    int diffoffset = 8;
    Type *tmp = NULL;
    while(consume("*")) {
        tmp = calloc(1, sizeof(Type));
        tmp->ptr_to = type;
        tmp->ty = PTR;
        type = tmp;
    }
    Token *tok = consume_ident();
    if(!tok) {
        if (type->ty == VA_ARGS) {
            LVar *lvar = calloc(1, sizeof(LVar));
            lvar->type = type;
            lvar->next = locals;
            return lvar;
        }
        error_at(token->str, "non variable");
    }
    LVar *lvar = find_lvar(tok);
    if (lvar)
        error_at(tok->str, "duplicate");
    lvar = calloc(1, sizeof(LVar));
    lvar->next = locals;
    lvar->name = tok->str;
    lvar->len = tok->len;
    if (consume("[")) {
        Type *tmp;
        tmp = type;
        type = calloc(1, sizeof(Type));
        type->ptr_to = tmp;
        type->ty = ARRAY;
        if (consume("]")) {
            lvar->type = type;
            lvar->offset = -1;
            return lvar;
        }

        Node *node = logic_or();
        expect("]");
        type->array_size = calc(node);
        diffoffset = sizeof_parse(type);
    }

    lvar->offset = locals->offset + diffoffset;
    if (lvar->offset % 8 != 0) {
        lvar->offset += (8 - lvar->offset % 8);
    }
    lvar->type = type;
    locals = lvar;
    return lvar;
}

void assign_lvar_arr(LVar *lvar, int len) {
    lvar->type->array_size = len;
    lvar->offset = locals->offset + sizeof_parse(lvar->type);
    if (lvar->offset % 8 != 0) {
        lvar->offset += (8 - lvar->offset % 8);
    }
    locals = lvar;
}

GVar *init_gvar(Type *type) {
    Type *tmp = NULL;
    while(consume("*")) {
        tmp = calloc(1, sizeof(Type));
        tmp->ptr_to = type;
        tmp->ty = PTR;
        type = tmp;
    }
    Token *tok = consume_ident();
    if(!tok) {
        error_at(token->str, "non variable");
    }
    GVar *lvar = find_gvar(tok);
    if (lvar)
        error_at(tok->str, "duplicate");
    lvar = calloc(1, sizeof(GVar));
    lvar->name = tok->str;
    lvar->len = tok->len;
    if (consume("[")) {
        Type *tmp = type;
        type = calloc(1, sizeof(Type));
        type->ptr_to = tmp;
        type->ty = ARRAY;
        if (consume("]")) {
            lvar->type = type;
            lvar->offset = -1;
            return lvar;
        }

        Node *node = logic_or();
        expect("]");
        type->array_size = calc(node);
    }
    lvar->offset = sizeof_parse(type);
    lvar->type = type;
    return lvar;
}

void init_gvar_arr(GVar *gvar, int len) {
    gvar->type->array_size = len;
    gvar->offset = sizeof_parse(gvar->type);
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

void program() {
    GVar **tmp = &code;
    globalstmt(tmp);
    while(!at_eof()) {
        tmp = &(*tmp)->next;
        globalstmt(tmp);
    }
}

void globalstmt(GVar **ptr) {
    Type *type = eval_type();
    Type *ttmp;
    while(consume("*")) {
        ttmp = calloc(1, sizeof(Type));
        ttmp->ptr_to = type;
        ttmp->ty = PTR;
        type = ttmp;
    }
    Token *prev = token;
    Token *tok = consume_ident();
    LVar *lvar, *tmp;
    if(!tok) error("invalid token");
    if (consume("(")) {
        GVar *func = calloc(1, sizeof(GVar));
        func->name = tok->str;
        func->len = tok->len;
        Type *ftype = calloc(1, sizeof(Type));
        func->type = ftype;
        ftype->ret = type;
        ftype->ty = FUNC;
        locals = calloc(1, sizeof(LVar));
        int i = 0;
        if (!consume(")")) {
            while(1) {
                type = eval_type();
                lvar = assign_lvar(type);
                if (type->ty == VA_ARGS) {
                    break;
                }
                i++;
                if(!consume(",")) break;
            }
            ftype->arglen = i;
            ftype->args = lvar;
            tmp = locals;
            for(lvar = locals; lvar; lvar = lvar->next) {
                if (lvar->offset > 6 * 8) {
                    lvar->offset = 5 * 8 - lvar->offset;
                }
                if (lvar->offset == -16){
                    locals = lvar->next;
                    lvar->next = NULL;
                    for(lvar = locals; lvar->next; lvar = lvar->next);
                    lvar->next = tmp;
                    break;
                }
            }
            expect(")");
        }
        tok = token;
        *ptr = func;
        if (consume(";")) {
            func->locals = locals;
            return;
        } else if (consume("{")) {
            token = tok;
            func->node = calc_node(stmt());
            func->locals = locals;
            return;
        } else {
            error_at(token->str, "関数ではありません");
        }
    } else {
        token = prev;
        GVar *gvar = init_gvar(type);
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
                    if (gvar->offset != -1) {
                        if (i > gvar->type->array_size) {
                            error_at(token->str, "引数が多すぎます");
                        }
                    } else {
                        init_gvar_arr(gvar, i);
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
                    if (gvar->offset != -1) {
                        if (i > gvar->type->array_size) {
                            error_at(token->str, "文字列が長すぎます");
                        }
                    } else {
                        init_gvar_arr(gvar, i);
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
        if (gvar->offset == -1) {
            error_at(token->str, "配列の初期化ができません");
        }
        expect(";");
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
    }
    else {
        node = expr();
        expect(";");
    }
    return node;
}

// (int | char) indent ([number])? (= assign | )? | assign
Node *expr() {
    if(token->kind == TK_INT || token->kind == TK_CHAR) {
        Type *type = eval_type();
        LVar *lvar = assign_lvar(type);
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
                        if (lvar->offset != -1) {
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
                        if (lvar->offset != -1) {
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
                    if (lvar->offset != -1) {
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
        return new_node(ND_DECLARATION, node, NULL, NULL);
    }
    return assign();
}

// logic_or ( = assign )*
Node *assign() {
    Node *node = logic_or();

    if(consume("=")) {
        Node *right = assign();
        node = assign_node(node, right);
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
            if(node->type->ty == PTR && right->type->ty == PTR)
                node = new_node(ND_SUB, node, right, &int_type);
            else if(node->type->ty == PTR)
                node = new_node(ND_SUB, node, new_node(ND_MUL, new_node_num(sizeof_parse(node->type->ptr_to)), right, NULL), node->type);
            else if(right->type->ty == PTR)
                error_at(token->str, "int - ptrは未定義です");
            else
                node = new_node(ND_SUB, node, right, &int_type);
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

// ("+" | "-")? primary (("[" assign "]")* | "(" assign ")" )  |  ("*" | "&") unary  |  sizeof unary
Node *unary() {
    Node *node;
    Type *tmp;
    if (consume("+"))
        return primary();
    if (consume("-")) {
        node = primary();
        if (node->type->ty == PTR)
            error_at(token->str, "-ptr is not defined");
        return new_node(ND_SUB, new_node_num(0), node, &int_type);
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