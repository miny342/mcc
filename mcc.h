#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// tokenize.c

// トークンの種類
typedef enum {
    TK_RESERVED, // 記号
    TK_IDENT,    // 識別子
    TK_NUM,      // 整数トークン
    TK_EOF,      // 入力の終わりを表すトークン
    TK_RETURN,   // return
    TK_IF,       // if
    TK_ELSE,     // else
    TK_WHILE,    // while
    TK_FOR,      // for
    TK_INT,      // int
} TokenKind;

typedef struct Token Token;

// トークン型
struct Token {
    TokenKind kind; // トークンの型
    Token *next;    // 次の入力トークン
    int val;        // kindがTK_NUMの場合、その数値
    char *str;      // トークン文字列
    int len;        // トークンの長さ
};

// 現在着目しているトークン
extern Token *token;

// 入力プログラム
extern char *user_input;

void error_at(char *loc, char *fmt, ...);
void error(char *fmt, ...);
bool consume(char *op);
void expect(char *op);
int expect_number();
bool at_eof();
Token *new_token(TokenKind kind, Token *cur, char *str, int len);
Token *tokenize(char *p);

// parse.c

typedef struct LVar LVar;

// local variable
struct LVar {
    LVar *next;  // 次の変数 or NULL
    char *name;  // 変数名
    int len;     // 名前の長さ
    int offset;  // RBPからのoffset
};

extern LVar *locals;

typedef enum {
    ND_ADD, // +
    ND_SUB, // -
    ND_MUL, // *
    ND_DIV, // /
    ND_EQ,  // ==
    ND_NE, // !=
    ND_LT, // <
    ND_LE, // <=
    ND_ASSIGN,  // =
    ND_LVAR,  // 変数
    ND_NUM, // integer
    ND_RETURN, // return
    ND_IF,   // if
    ND_ELSE, // else
    ND_WHILE, // while
    ND_FOR, // for
    ND_BLOCK, // {block}
    ND_CALL,  // call function
    ND_ADDR,  // address
    ND_DEREF,  // デリファレンス
} NodeKind;

typedef struct Node Node;

// 抽象構文木のノードの型
struct Node {
    NodeKind kind; // ノードの型
    Node *lhs;     // 左辺
    Node *rhs;     // 右辺
    int val;       // kind == ND_NUM
    int offset;    // kind == ND_LVAR
    char *name;    // kind == ND_CALL
    int len;       // kind == ND_CALL
};

typedef struct Global Global;

struct Global {
    Global *next;  // next Global or NULL
    Node *node;    // stmt
    LVar *locals;  // local variable
    int arglen;    // function args (not float) quantity
    char *name;    // function
    int len;       // function
};

extern Global *code;

Global *globalstmt();
Node *new_node(NodeKind kind, Node *lhs, Node *rhs);
Node *new_node_num(int val);
void program();
Node *stmt();
Node *expr();
Node *assign();
Node *equality();
Node *relational();
Node *add();
Node *mul();
Node *unary();
Node *primary();


// codegen.c
void gen(Node *node);
void gen_global();

extern int loopcnt;
