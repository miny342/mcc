#include "libc_alternatives.h"

// tokenize.c


typedef struct Vec Vec;

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
    TK_STRUCT,   // struct
    TK_ENUM,     // enum
    TK_ONE_CHAR, // 一文字
    TK_TYPEDEF,  // typedef
    TK_MACRO,    // #
    TK_MACRO_END, // #の後の改行
    TK_EXTERN,   // extern
    TK_SWITCH,   // switch
    TK_CASE,     // case
    TK_DEFAULT,  // default
    TK_DO,       // do
    TK_SHORT,    // short
    TK_STATIC,   // static
} TokenKind;

typedef struct Token Token;

// トークン型
struct Token {
    TokenKind kind; // トークンの型
    Token *next;    // 次の入力トークン
    int val;        // kindがTK_NUMの場合、その数値
    char *str;      // トークン文字列
    int len;        // トークンの長さ
    char *error;    // エラー時の場所
};

// 現在着目しているトークン
extern Token *token;

typedef struct {
    Vec *args;
    Token *expand_to;
} MacroData;

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
Token *tokenize(char *p, int need_eof);
Token *preprocess();
void print_token(Token *t);

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
        STRUCT,
        SHORT,
    } ty;
    Type *ptr_to;
    int array_size;
    Type *ret; // ty == FUNCの時の返り値
    Vec *args; // ty == FUNCの時の引数
    Token *tok; // argsや構造体の名前など
    int offset; // ty == STRUCTの時のoffset
    Vec *fields; // ty == STRUCTの時のフィールド
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
extern int max_offset;

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
    int offset;  // type == func
    int is_extern; // externかどうか
    int is_static; // staticかどうか
    int is_globl;  // globlにするか
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
    ND_GVAR, // グローバル変数
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
    ND_SWITCH, // switch
    ND_CASE,  // case
    ND_DEFAULT, // default
    ND_DO,  // do
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
    GVar *gvar;    // kind == ND_GVAR
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
Node *cast();
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

void *strmapget(StrMap *map, unsigned char *str, int len);
int strmapset(StrMap *map, unsigned char *str, int len, void *value);

extern StrMap *strmap;
extern StrMap *structmap;
extern StrMap *enummap;
extern StrMap *enumkeymap;
extern StrMap *typenamemap;
extern StrMap *macromap;

struct Vec {
    void **data;
    int len;
    int cap;
};

void *at(Vec *v, int i);
void push(Vec *v, void *i);
Vec *vec_new();

extern int pushcnt;

char *read_file(char *path);
