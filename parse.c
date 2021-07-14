#include "mcc.h"

static Type int_type = {
    INT,
    NULL
};

// エラー箇所を報告する
void error_at(char *loc, char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    int pos = loc - user_input;
    fprintf(stderr, "%s\n", user_input);
    fprintf(stderr, "%*s", pos, " "); // pos個の空白
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
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

        if (strncmp(p, "if", 2) == 0 && !is_alnum(p[2])) {
            cur = new_token(TK_IF, cur, p, 2);
            p += 2;
            continue;
        }

        if (strncmp(p, "==", 2) == 0 || strncmp(p, "!=", 2) == 0 ||
            strncmp(p, "<=", 2) == 0 || strncmp(p, ">=", 2) == 0) {
                cur = new_token(TK_RESERVED, cur, p, 2);
                p += 2;
                continue;
            }

        if (isalpha(*p)) {
            lvar_length = 0;
            while(is_alnum(p[lvar_length]))
                lvar_length++;
            cur = new_token(TK_IDENT, cur, p, lvar_length);
            p += lvar_length;
            continue;
        }

        if (strchr("+-*/()<>;={},&", *p)) {
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

Type *eval_type() {
    if(!consumeTK(TK_INT))
        error_at(token->str, "no int");
    Type *tmp;
    Type *type = calloc(1, sizeof(Type));
    type->ty = INT;
    while(consume("*")) {
        tmp = calloc(1, sizeof(Type));
        tmp->ptr_to = type;
        tmp->ty = PTR;
        type = tmp;
    }
    return type;
}

LVar *assign_lvar(Type *type) {
    Token *tok = consume_ident();
    if(!tok)
        error_at(token->str, "non variable");
    LVar *lvar = find_lvar(tok);
    if (lvar)
        error_at(tok->str, "duplicate");
    lvar = calloc(1, sizeof(LVar));
    lvar->next = locals;
    lvar->name = tok->str;
    lvar->len = tok->len;
    lvar->offset = locals->offset + 8;
    lvar->type = type;
    locals = lvar;
    return lvar;
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

void program() {
    code = globalstmt();
    Global *tmp = code;
    while(!at_eof()) {
        tmp->next = globalstmt();
        tmp = tmp->next;
    }
}

Global *globalstmt() {
    Type *type = eval_type();
    Token *tok = consume_ident();
    LVar *lvar, *tmp;
    if(!tok) error("this is not function");
    Global *glob = calloc(1, sizeof(Global));
    expect("(");
    glob->name = tok->str;
    glob->len = tok->len;
    glob->type = type;
    locals = calloc(1, sizeof(LVar));
    if(!consume(")")) {
         for(;;) {
            type = eval_type();
            assign_lvar(type);
            glob->arglen += 1;
            if(!consume(",")) break;
        }
        tmp = locals;
        for(lvar = locals; lvar; lvar = lvar->next) {
            if(lvar->offset > 6 * 8)
                lvar->offset = 5 * 8 - lvar->offset;
            if(lvar->offset == -16) {
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
    expect("{");
    token = tok;
    glob->node = stmt();
    glob->locals = locals;
    return glob;
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
        }
        if(!consume(")")) {
            node->rhs->rhs->lhs = expr();
            expect(")");
        }
        node->rhs->rhs->rhs = stmt();
    } else if (consume("{")) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_BLOCK;
        Node *now_node = node;
        while(!consume("}")) {
            now_node->lhs = stmt();
            now_node->rhs = calloc(1, sizeof(Node));
            now_node = now_node->rhs;
            now_node->kind = ND_BLOCK;
        }
    }
    else {
       node = expr();
       expect(";");
    }
    return node;
}

Node *expr() {
    if(token->kind == TK_INT) {
        Type *type = eval_type();
        assign_lvar(type);
        return new_node_num(0);
    }
    return assign();
}

Node *assign() {
    Node *node = equality();

    if(consume("=")) {
        Node *right = assign();
        node = new_node(ND_ASSIGN, node, right, right->type);
    }
    return node;
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

// add ("<" add | "<=" add | ">" add | ">=" add)*
Node *relational() {
    Node *node = add();

    for(;;) {
        if (consume("<="))
            node = new_node(ND_LE, node, add(), &int_type);
        else if (consume(">="))
            node = new_node(ND_LE, add(), node, &int_type);
        else if (consume("<"))
            node = new_node(ND_LT, node, add(), &int_type);
        else if (consume(">"))
            node = new_node(ND_LT, add(), node, &int_type);
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
            if(node->type->ty == PTR && right->type->ty == PTR) {
                error_at(token->str, "ptr + ptrは未定義です");
            }
            else if(node->type->ty == PTR && node->type->ptr_to->ty == INT)
                node = new_node(ND_ADD, node, new_node(ND_MUL, new_node_num(4), right, &int_type), node->type);
            else if(node->type->ty == PTR && node->type->ptr_to->ty == PTR)
                node = new_node(ND_ADD, node, new_node(ND_MUL, new_node_num(8), right, &int_type), node->type);
            else if(right->type->ty == PTR && right->type->ptr_to->ty == INT)
                node = new_node(ND_ADD, new_node(ND_MUL, new_node_num(4), node, &int_type), right, right->type);
            else if(right->type->ty == PTR && right->type->ptr_to->ty == PTR)
                node = new_node(ND_ADD, new_node(ND_MUL, new_node_num(8), node, &int_type), right, right->type);
            else
                node = new_node(ND_ADD, node, right, &int_type);
        }
        else if (consume("-")) {
            right = mul();
            if(node->type->ty == PTR && right->type->ty == PTR)
                error_at(token->str, "ptr - ptrは未定義です");
            else if(node->type->ty == PTR && node->type->ptr_to->ty == INT)
                node = new_node(ND_SUB, node, new_node(ND_MUL, new_node_num(4), right, &int_type), node->type);
            else if(node->type->ty == PTR && node->type->ptr_to->ty == PTR)
                node = new_node(ND_SUB, node, new_node(ND_MUL, new_node_num(8), right, &int_type), node->type);
            else if(right->type->ty == PTR && right->type->ptr_to->ty == INT)
                node = new_node(ND_SUB, new_node(ND_MUL, new_node_num(4), node, &int_type), right, right->type);
            else if(right->type->ty == PTR && right->type->ptr_to->ty == PTR)
                node = new_node(ND_SUB, new_node(ND_MUL, new_node_num(8), node, &int_type), right, right->type);
            else
                node = new_node(ND_SUB, node, right, &int_type);
        }
        else
            return node;
    }
}

// unary ( * unary | / unary )*
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
        else
            return node;
    }
}

// ("+" | "-")? primary  |  ("*" | "&") unary
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
        tmp = calloc(1, sizeof(Type));
        tmp->ptr_to = node->type;
        tmp->ty = PTR;
        return new_node(ND_ADDR, node, NULL, tmp);
    }
    return primary();
}

// num | "(" expr ")" | ident ("(" ")")?
Node *primary() {
    if (consume("(")){
        Node *node = expr();
        expect(")");
        return node;
    }

    Token *tok = consume_ident();
    if(tok) {
        Node *node = calloc(1, sizeof(Node));
        if(consume("(")) {
            node->kind = ND_CALL;
            node->name = tok->str;
            node->len = tok->len;
            node->type = &int_type; // TODO: fix
            Node *now_node = node;
            if(!consume(")")) {
                for(;;) {
                    now_node->lhs = expr();
                    now_node->rhs = calloc(1, sizeof(Node));
                    now_node = now_node->rhs;
                    if(!consume(",")) break;
                }
                expect(")");
            }
            return node;
        }
        node->kind = ND_LVAR;
        LVar *lvar = find_lvar(tok);
        if (lvar) {
            node->lvar = lvar;
            node->type = lvar->type;
        } else 
            error_at(tok->str, "undefined");
        return node;
    }

    return new_node_num(expect_number());
}