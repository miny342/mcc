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
    TK_VOID,     // void
    TK_BREAK,    // break
    TK_CONTINUE, // continue
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
void warning_at(char *loc, char *fmt, ...);
void error(char *fmt, ...);
bool consume(char *op);
void expect(char *op);
int expect_number();
bool at_eof();
Token *new_token(TokenKind kind, Token *cur, char *str, int len);
Token *tokenize(char *p);

// parse.c

typedef struct Node Node;

typedef struct Type Type;

typedef struct LVar LVar;

struct Type {
    enum {
        INT,
        PTR,
        ARRAY,
        CHAR,
        FUNC,
        VA_ARGS, // ...
        VOID,
    } ty;
    Type *ptr_to;
    int array_size;
    Type *ret; // ty == FUNCの時の返り値
    Type *args; // ty == FUNCの時の引数の列
    int arglen; // ty == FUNCの時の引数の数
    Token *tok; // argsや構造体の名前など
};

// local variable
struct LVar {
    LVar *next;  // 次の変数 or NULL
    char *name;  // 変数名
    int len;     // 名前の長さ
    int offset;  // RBPからのoffset
    Type *type;   // 変数の型
};

extern LVar *locals;

typedef struct GVar GVar;

// global variable
struct GVar {
    GVar *next;
    char *name;
    int len;
    int size; // type != func
    Type *type;
    Node *node; // GVarの初期値またはtype==funcの時の実行ノード
    LVar *locals; // type == func
};

extern GVar *code;
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
    ND_DECLARATION, // 変数宣言
    ND_NEG, // -a
    ND_INCR, // ++
    ND_DECR, // --
    ND_NOT, // !
    ND_BITNOT, // ~
    ND_CONTINUE, // continue
    ND_BREAK, // break
} NodeKind;

// 抽象構文木のノードの型
struct Node {
    NodeKind kind; // ノードの型
    Node *lhs;     // 左辺
    Node *rhs;     // 右辺
    Type *type;    // 計算時のtype
    int val;       // kind == ND_NUM
    LVar *lvar;    // kind == ND_LVAR
    String *s;     // kind == ND_STR
    GVar *gvar;    // kind == ND_GLOVAL_LVAR
};

Type *eval_type_acc(Token **ident, Type **bottom);
Type *eval_type_all();
void type_test();

int sizeof_parse(Type *type);
int get_align(Type *type);
int calc_aligned(int offset, Type *type);
void globalstmt(GVar **ptr);
Node *new_node(NodeKind kind, Node *lhs, Node *rhs, Type *type);
Node *new_node_num(int val);
void program();
Node *stmt();
Node *expr();
Node *assign();
Node *if_op();
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
int gen_gvar(Node *node);

extern int labelcnt;
extern int continue_label; // 0ならループ外、1ならループ内、それ以外はジャンプ先
extern int break_label; // 同様

// tool

typedef struct StrMap StrMap;

struct StrMap {
    StrMap *p[256];
    void *value;
};

void *strmapget(StrMap *map, char *str, int len);
void strmapset(StrMap *map, char *str, int len, void *value);

extern StrMap *strmap;
