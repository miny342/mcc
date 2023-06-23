#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

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
    TK_SIZEOF,   // sizeof
    TK_CHAR,     // char
    TK_STR,      // string
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

extern char *filename;

void error_at(char *loc, char *fmt, ...);
void error(char *fmt, ...);
bool consume(char *op);
void expect(char *op);
int expect_number();
bool at_eof();
Token *new_token(TokenKind kind, Token *cur, char *str, int len);
Token *tokenize(char *p);

// parse.c

typedef struct Type Type;

struct Type {
    enum {INT, PTR, ARRAY, CHAR} ty;
    Type *ptr_to;
    size_t array_size;
};

typedef struct LVar LVar;

// local variable
struct LVar {
    LVar *next;  // 次の変数 or NULL
    char *name;  // 変数名
    int len;     // 名前の長さ
    int offset;  // RBPからのoffset
    Type *type;   // 変数の型
};

extern LVar *locals;
extern LVar *globals;

typedef struct String String;

struct String {
    String *next;  // 次の変数
    Token *tok;  // 文字列のトークン
    int offset;  // ラベルoffset
};

extern String *strs;

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
    ND_GLOVAL_LVAR, // グローバル変数
    ND_STR, // 文字列
    ND_REMINDER, // %
    ND_LSHIFT, // <<
    ND_RSHIFT, // >>
    ND_BITAND, // &
    ND_BITXOR, // ^
    ND_BITOR,  // |
    ND_AND, // &&
    ND_OR, // ||

} NodeKind;

typedef struct Node Node;

// 抽象構文木のノードの型
struct Node {
    NodeKind kind; // ノードの型
    Node *lhs;     // 左辺
    Node *rhs;     // 右辺
    Type *type;    // 計算時のtype
    int val;       // kind == ND_NUM
    LVar *lvar;    // kind == ND_LVAR
    char *name;    // kind == ND_CALL
    int len;       // kind == ND_CALL
    String *s;     // kind == ND_STR
};

typedef struct Function Function;

struct Function {
    Function *next;  // next Function or NULL
    Node *node;    // stmt
    LVar *locals;  // local variable
    int arglen;    // function args (not float) quantity
    char *name;    // function
    int len;       // function
    Type *type;    // return type
};

extern Function *code;

int sizeof_parse(Type *type);
Function *globalstmt();
Node *new_node(NodeKind kind, Node *lhs, Node *rhs, Type *type);
Node *new_node_num(int val);
void program();
Node *stmt();
Node *expr();
Node *assign();
Node *logic_or();
Node *logic_and();
Node *bit_or();
Node *bit_xor();
Node *bit_and();
Node *equality();
Node *relational();
Node *shift();
Node *add();
Node *mul();
Node *unary();
Node *primary();


// codegen.c
void gen(Node *node);
void gen_global();

extern int loopcnt;
