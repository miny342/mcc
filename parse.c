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
        error_at(token->error, "'%s'ではありません", op);
    token = token->next;
}

// 次のトークンが数値の場合、トークンを1つ読み進めてその数値を返す。
// それ以外の場合にはエラーを報告する。
int expect_number() {
    if (token->kind != TK_NUM) error_at(token->error, "数ではありません");
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

// Cの意味で一文字読む
char read_one_char(char **top) {
    if (**top == '\\') {
        (*top)++;
        if (**top == 'n') {
            (*top)++;
            return '\n';
        } else if (**top == '\\') {
            (*top)++;
            return '\\';
        } else if (**top == '"') {
            (*top)++;
            return '"';
        } else if (**top == '\'') {
            (*top)++;
            return '\'';
        } else if (**top == '0') {
            (*top)++;
            return 0;
        } else {
            error_at(*top, "charとして処理できません");
        }
    }
    (*top)++;
    return *(*top - 1);
}

// 新しいトークンを作成してcurに繋げる
Token *new_token(TokenKind kind, Token *cur, char *str, int len) {
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->str = str;
    tok->len = len;
    cur->next = tok;
    tok->error = str;
    return tok;
}

// 入力文字列pをトークナイズしてそれを返す
Token *tokenize(char *p, int need_eof) {
    Token head;
    head.next = NULL;
    Token *cur = &head;
    int lvar_length;

    int is_reading_macro = 0;

    while(*p) {
        if (is_reading_macro && *p == '\\' && p[1] == '\n') {
            p += 2;
            continue;
        }

        if (is_reading_macro && *p == '\n') {
            is_reading_macro = 0;
            cur = new_token(TK_MACRO_END, cur, p, 1);
            p += 1;
            continue;
        }

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

        if (strncmp(p, "typedef", 7) == 0 && !is_alnum(p[7])) {
            cur = new_token(TK_TYPEDEF, cur, p, 7);
            p += 7;
            continue;
        }

        if (strncmp(p, "default", 7) == 0 && !is_alnum(p[7])) {
            cur = new_token(TK_DEFAULT, cur, p, 7);
            p += 7;
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

        if (strncmp(p, "struct", 6) == 0 && !is_alnum(p[6])) {
            cur = new_token(TK_STRUCT, cur, p, 6);
            p += 6;
            continue;
        }

        if (strncmp(p, "extern", 6) == 0 && !is_alnum(p[6])) {
            cur = new_token(TK_EXTERN, cur, p, 6);
            p += 6;
            continue;
        }

        if (strncmp(p, "switch", 6) == 0 && !is_alnum(p[6])) {
            cur = new_token(TK_SWITCH, cur, p, 6);
            p += 6;
            continue;
        }

        if (strncmp(p, "static", 6) == 0 && !is_alnum(p[6])) {
            cur = new_token(TK_STATIC, cur, p, 6);
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

        if (strncmp(p, "short", 5) == 0 && !is_alnum(p[5])) {
            cur = new_token(TK_SHORT, cur, p, 5);
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

        if (strncmp(p, "enum", 4) == 0 && !is_alnum(p[4])) {
            cur = new_token(TK_ENUM, cur, p, 4);
            p += 4;
            continue;
        }

        if (strncmp(p, "case", 4) == 0 && !is_alnum(p[4])) {
            cur = new_token(TK_CASE, cur, p, 4);
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

        if (strncmp(p, "do", 2) == 0 && !is_alnum(p[2])) {
            cur = new_token(TK_DO, cur, p, 2);
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
            char *tmp = p + 1;
            while (*tmp != '\"') read_one_char(&tmp);
            if (!tmp) error_at(p, "\" が見つかりません");
            tmp++;
            cur = new_token(TK_STR, cur, p, tmp - p);
            p = tmp;
            continue;
        }

        if (*p == '\'') {
            char *tmp = p + 1;
            read_one_char(&tmp);
            if (*tmp != '\'') {
                error_at(p, "一文字ではありません");
            }
            tmp++;
            cur = new_token(TK_ONE_CHAR, cur, p, tmp - p);
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
            strncmp(p, "|=", 2) == 0 || strncmp(p, "^=", 2) == 0 ||
            strncmp(p, "->", 2) == 0) {
                cur = new_token(TK_RESERVED, cur, p, 2);
                p += 2;
                continue;
            }

        if (*p == '#') {
            is_reading_macro = 1;
            cur = new_token(TK_MACRO, cur, p, 1);
            p++;
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

        if (strchr("+-*/()<>;={},&[]%^|?:~!.", *p)) {
            cur = new_token(TK_RESERVED, cur, p++, 1);
            continue;
        }

        if (isdigit(*p)) {
            char *tmp = p;
            while(isdigit(*tmp)) tmp++;
            cur = new_token(TK_NUM, cur, p, tmp - p);
            cur->val = strtol(p, &p, 10);
            continue;
        }

        error_at(p, "トークナイズできません");
    }

    if (need_eof) {
        new_token(TK_EOF, cur, p, 0);
    }
    return head.next;
}

// NULLに行きつくまで複製
Token *duplicate_token(Token *tok, char *error) {
    if (tok == NULL) {
        return NULL;
    }
    Token *t = malloc(sizeof(Token));
    memcpy(t, tok, sizeof(Token));
    t->error = error;
    t->next = duplicate_token(tok->next, error);
    return t;
}

// kindとなるトークンまで読み飛ばし、そのトークンを返す
// もし存在しないならエラー
Token *expect_exist(TokenKind kind) {
    Token *errtok = token;
    while(token && token->kind != kind) token = token->next;
    if (token) {
        Token *tmp = token;
        token = token->next;
        return tmp;
    } else {
        error_at(errtok->error, "expected %d token kind, but not found", kind);
    }
}

// tokenに入ったTokenを処理してTokenを返す
Token *preprocess() {
    // Token *head = token; // 退避
    Token head;
    head.next = NULL;
    Token *prev = &head; // tokenのひとつ前を保持

    while (1) {
        if (token == NULL) {
            break;
        }
        Token *ident = consume_ident();
        if (ident) {
            MacroData *data = strmapget(macromap, ident->str, ident->len);
            if (data) {
                strmapset(macromap, ident->str, ident->len, NULL); // 再帰的な展開を抑えるために一時的に未定義にする マクロ内でマクロ宣言などはできなくてよい
                Token *prev_in_macro;
                Vec *tmpv = vec_new();
                if (data->args && consume("(")) {
                    if (data->args->len > 0) {
                        int kakko = 0;
                        push(tmpv, token);
                        while(1) {
                            prev_in_macro = token;
                            if (consume("(")) {
                                kakko++;
                            } else if (consume(")")) {
                                if (kakko == 0 && data->args->len <= tmpv->len) {
                                    prev_in_macro->next = NULL;
                                    break;
                                }
                                if (kakko == 0) {
                                    error_at(token->error, "マクロの引数が足りません");
                                }
                                kakko--;
                            } else if (kakko == 0 && consume(",")) {
                                prev_in_macro->next = NULL;
                                push(tmpv, token);
                            } else {
                                token = token->next;
                            }
                        }
                    }
                    if (tmpv->len > data->args->len) {
                        error_at(token->error, "引数が多すぎます");
                    }

                    Token *replace_to = duplicate_token(data->expand_to, ident->error);
                    prev_in_macro = NULL;
                    // argを変換
                    for (Token *tok = replace_to; tok; tok = tok->next) {
                        if (tok->kind == TK_IDENT) {
                            for (int i = 0; i < data->args->len; i++) {
                                Token *arg = data->args->data[i];
                                if (arg->len == tok->len && !strncmp(arg->str, tok->str, arg->len)) {
                                    Token *arg_replace_to = duplicate_token(tmpv->data[i], ident->error);
                                    if (!prev_in_macro) {
                                        replace_to = arg_replace_to;
                                        prev_in_macro = arg_replace_to;
                                    } else {
                                        prev_in_macro->next = arg_replace_to;
                                    }
                                    while(arg_replace_to->next) {
                                        prev_in_macro = arg_replace_to;
                                        arg_replace_to = arg_replace_to->next;
                                    }
                                    prev_in_macro->next = tok->next;
                                    break;
                                }
                            }
                        }

                        prev_in_macro = tok;
                    }

                    prev_in_macro = NULL;
                    // argが変換された物をみて文字列操作
                    for (Token *tok = replace_to; tok; tok = tok->next) {
                        if (tok->kind == TK_MACRO) {
                            if (prev_in_macro == NULL) {
                                error_at(ident->error, "#が先頭にあります");
                            }
                            if (tok->next->kind == TK_MACRO) {
                                tok = tok->next->next;
                                int len = prev_in_macro->len + tok->len;
                                char *tmp = malloc(len + 1);
                                memcpy(tmp, prev_in_macro->str, prev_in_macro->len);
                                memcpy(tmp + prev_in_macro->len, tok->str, tok->len);
                                tmp[len] = 0;
                                Token *t = tokenize(tmp, 1);
                                t->error = ident->error;
                                prev_in_macro->kind = t->kind;
                                prev_in_macro->str = t->str;
                                prev_in_macro->len = t->len;
                                prev_in_macro->next = t->next;
                                prev_in_macro->error = t->error;
                                if (t->next->kind != TK_EOF) {
                                    while(t->next) {
                                        prev_in_macro = t;
                                        t->error = ident->error;
                                        t = t->next;
                                    }
                                }
                                prev_in_macro->next = tok->next;
                                continue;
                            } else {
                                // TODO: エスケープする
                                tok = tok->next;
                                prev_in_macro->next = tok;
                                tok->kind = TK_STR;
                                tok->len += 2;
                                char *tmp = malloc(tok->len + 1);
                                tmp[0] = '\"';
                                tmp[tok->len - 1] = '\"';
                                tmp[tok->len] = 0;
                                memcpy(tmp + 1, tok->str, tok->len - 2);
                                tok->str = tmp;
                                tok->error = ident->error;
                            }
                        }
                        prev_in_macro = tok;
                    }

                    if (replace_to != NULL) {
                        Token *now_token = token;
                        token = replace_to;
                        replace_to = preprocess();
                        token = now_token;

                        prev->next = replace_to;
                        while(replace_to->next) replace_to = replace_to->next;
                        replace_to->next = token;
                        prev = replace_to;
                    } else {
                        prev->next = ident->next;
                    }
                    strmapset(macromap, ident->str, ident->len, data);
                    continue;
                } else if (!data->args) {
                    strmapset(macromap, ident->str, ident->len, NULL);

                    Token *replace_to = duplicate_token(data->expand_to, ident->error);
                    if (replace_to != NULL) {
                        Token *now_token = token;
                        token = replace_to;
                        replace_to = preprocess();
                        token = now_token;

                        prev->next = replace_to;
                        while(replace_to->next) replace_to = replace_to->next;
                        replace_to->next = token;
                        prev = replace_to;
                    } else {
                        prev->next = ident->next;
                    }
                    strmapset(macromap, ident->str, ident->len, data);
                    continue;
                } else {
                    prev->next = ident;
                    prev = ident;
                    token = ident->next;
                    continue;
                }
            } else {
                prev->next = ident;
                prev = ident;
                token = ident->next;
                continue;
            }
        } else if (consumeTK(TK_MACRO)) {
            Token *tok = consume_ident();
            if (tok->len == 6 && !strncmp(tok->str, "define", 6)) {
                tok = consume_ident();
                MacroData *data = calloc(1, sizeof(MacroData));
                if (tok->str[tok->len] == '(' && consume("(")) { // identと(はくっついている必要がある
                    data->args = vec_new();  // 関数マクロかはargsの初期化で見分ける
                    if (!consume(")")) {
                        while(1) {
                            Token *arg = consume_ident();
                            if (!arg) {
                                error_at(token->error, "不正なトークンです");
                            }
                            push(data->args, arg); // トークンを指すアドレスを入れておく
                            if (!consume(",")) {
                                expect(")");
                                break;
                            }
                        }
                    }
                }
                if (token->kind != TK_MACRO_END) {
                    data->expand_to = token;

                    while (token->next->kind != TK_MACRO_END) {
                        token = token->next;
                    }
                    Token *tmp = token->next;
                    token->next = NULL;
                    token = tmp;
                }
                if (!consumeTK(TK_MACRO_END)) {
                    error_at(token->error, "unexpected error(compiler bug)");
                }
                prev->next = token;
                strmapset(macromap, tok->str, tok->len, data);
                continue;
            } else if (tok->len == 7 && !strncmp(tok->str, "include", 7)) {
                char *filename;
                char *filetext;
                switch (token->kind) {
                    case TK_STR:
                        filename = malloc(token->len - 1);
                        memcpy(filename, token->str + 1, token->len - 2);
                        filename[token->len - 2] = 0;
                        fprintf(stderr, "%s\n", filename);
                        filetext = read_file(filename);
                        if (filetext) {
                            token = token->next;
                            break;
                        }
                    case TK_RESERVED:
                        if (consume("<")) {
                            int len = 0;
                            while (token->str[len] != '>') {
                                len++;
                            }
                            char loc[] = "/usr/include/";
                            filename = malloc(sizeof(loc) + len);
                            memcpy(filename, loc, sizeof(loc));
                            memcpy(filename + sizeof(loc) - 1, token->str, len);
                            filename[sizeof(loc) + len - 1] = 0;
                            fprintf(stderr, "%s\n", filename);
                            filetext = read_file(filename);
                            if (filetext) {
                                while (!consume(">")) {
                                    token = token->next;
                                }
                                break;
                            }
                        }
                    default:
                        error_at(token->error, "includeできません");
                }
                if (!consumeTK(TK_MACRO_END)) {
                    error_at(token->error, "不正なトークンを含んでいます");
                }

                Token *filetoken = tokenize(filetext, 0);
                Token *now_token = token;
                token = filetoken;
                filetoken = preprocess();
                token = now_token;

                if (filetoken) {
                    prev->next = filetoken;
                    while(filetoken->next) {
                        filetoken->error = tok->error;
                        filetoken = filetoken->next;
                    }
                    filetoken->next = token;
                    prev = filetoken;
                }

                continue;
            } else if (tok->len == 5 && !strncmp(tok->str, "ifdef", 5)) {
                tok = consume_ident();
                MacroData *data = strmapget(macromap, tok->str, tok->len);
                int ifcount = 0;
                if (token->kind != TK_MACRO_END) {
                    error_at(tok->error, "ifdefが正常に終わっていません");
                }
                token = token->next;
                Token *iftok = token; // start
                Token *elsetok = NULL; // 存在するなら#elseの#を指す
                Token *endiftok; // #endifの#
                while(1) {
                    Token *tmp = expect_exist(TK_MACRO);
                    tok = consume_ident();
                    if (!tok) {
                        Token *errtok = token;
                        if (consumeTK(TK_ELSE)) {
                            if (ifcount == 0) {
                                if (elsetok) {
                                    error_at(errtok->error, "二度目のelseマクロです");
                                }
                                if (token->kind != TK_MACRO_END) {
                                    error_at(errtok->error, "elseマクロが正常に終わっていません");
                                }
                                elsetok = tmp;
                            }
                            continue;
                        } else {
                            error_at(errtok->error, "token %dはidentではありません", errtok->kind);
                        }
                    }
                    if (tok->len == 5 && !strncmp(tok->str, "ifdef", 5)) {
                        ifcount++;
                    } else if (tok->len == 6 && !strncmp(tok->str, "ifndef", 6)) {
                        ifcount++;
                    } else if (tok->len == 5 && !strncmp(tok->str, "endif", 5)) {
                        if (ifcount > 0) {
                            ifcount--;
                        } else {
                            if (token->kind != TK_MACRO_END) {
                                error_at(token->error, "endifマクロが正常に終わっていません");
                            }
                            token = token->next;
                            endiftok = tmp;
                            break;
                        }
                    }
                }
                if (data) {
                    Token *tmp = iftok;
                    if (tmp && tmp != elsetok && tmp != endiftok) {
                        while (tmp->next != elsetok && tmp->next != endiftok) tmp = tmp->next;
                        tmp->next = NULL;
                        Token *nowtok = token;
                        token = iftok;
                        iftok = preprocess();
                        token = nowtok;
                        tmp = iftok;
                        if (tmp) while (tmp->next) tmp = tmp->next;
                        prev->next = iftok;
                        prev = tmp;
                    }
                    continue;
                } else if (elsetok) {
                    elsetok = elsetok->next->next->next;
                    Token *tmp = elsetok;
                    if (tmp && tmp != endiftok) {
                        while (tmp->next != endiftok) tmp = tmp->next;
                        tmp->next = NULL;
                        Token *nowtok = token;
                        token = elsetok;
                        elsetok = preprocess();
                        token = nowtok;
                        tmp = elsetok;
                        if (tmp) while (tmp->next) tmp = tmp->next;
                        prev->next = elsetok;
                        prev = tmp;
                    }
                    continue;
                } else {
                    prev->next = token;
                    continue;
                }
            } else {
                error_at(tok->str, "未定義のマクロです");
            }
        }
        prev->next = token;
        prev = token;
        token = token->next;
    }
    return head.next;
}

// token列をそれっぽく表示する
void print_token(Token *t) {
    int indent = 0;
    for (Token *tok = t; tok; tok = tok->next) {
        if (tok->kind == TK_NUM) {
            fprintf(stderr, "%d ", tok->val);
        } else if (tok->kind == TK_RESERVED) {
            if (*tok->str == '{') {
                indent++;
                fprintf(stderr, "{\n%*s", indent * 4, "");
            } else if (*tok->str == '}') {
                if (indent > 0) {
                    indent--;
                }
                fprintf(stderr, "\n%*s}\n%*s", indent * 4, "", indent * 4, "");
            } else if (*tok->str == ';') {
                fprintf(stderr, ";\n%*s", indent * 4, "");
            } else {
                fprintf(stderr, "%.*s ", tok->len, tok->str);
            }
        } else if (tok->kind == TK_EOF) {
            fprintf(stderr, "EOF ");
        } else {
            fprintf(stderr, "%.*s ", tok->len, tok->str);
        }
    }
    fprintf(stderr, "\n");
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
        case ND_IF:
            if (node->rhs->kind == ND_ELSE) {
                if (l) {
                    if (node->rhs->lhs->kind == ND_NUM) {
                        return node->rhs->lhs;
                    }
                } else {
                    if (node->rhs->rhs->kind == ND_NUM) {
                        return node->rhs->rhs;
                    }
                }
            }
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
        error_at(token->error, "calc error");
    }
    return n->val;
}

int sizeof_parse(Type *type) {
    if (!type)
        error_at(token->error, "type parse error");
    else if (type->ty == INT)
        return sizeof(int);
    else if (type->ty == PTR)
        return sizeof(int*);
    else if (type->ty == ARRAY)
        return type->array_size * sizeof_parse(type->ptr_to);
    else if (type->ty == CHAR || type->ty == VOID || type->ty == FUNC)
        return sizeof(char);
    else if (type->ty == SHORT)
        return sizeof(short);
    else if (type->ty == STRUCT) {
        if (!type->fields) {
            error("sizeofをフィールド不明な構造体に適用できません");
        }
        if (type->fields->len == 0) {
            return 0;
        }
        Type *ty = type->fields->data[type->fields->len - 1];
        int size = ty->offset + sizeof_parse(ty);
        int align = get_align(ty);
        return size + (align - (size % align)) % align;
    }
    else error_at(token->error, "不明な型のsizeof");
}

int get_align(Type *type) {
    if (!type)
        error_at(token->error, "type parse error");
    else if (type->ty == ARRAY)
        return get_align(type->ptr_to);
    else if (type->ty == STRUCT) {
        if (!type->fields) {
            error("get_alignをフィールド不明な構造体に適用できません");
        }
        int max = 1;
        for (int i = 0; i < type->fields->len; i++) {
            Type *ty = type->fields->data[i];
            int align = get_align(ty);
            if (align > max) {
                max = align;
            }
        }
        return max;
    } else return sizeof_parse(type);
}

// type.ty == STRUCTでtype.arglen == -1を受け取って初期化する
// consume("{")はされていると思って読む
Type *def_struct(Type *type) {
    if (type->ty != STRUCT) {
        error_at(token->error, "structではありません(コンパイラのバグ)");
    }
    if (type->fields) {
        error_at(token->error, "定義済みです");
    }
    Type *args = type;
    int offset = 0;
    type->fields = vec_new();
    if (!consume("}")) {
        while(1) {
            Type *ty = eval_type_all();
            if (!ty || !ty->tok) {
                error_at(token->error, "フィールド名が定義されていません");
            }
            expect(";");
            int size = sizeof_parse(ty);
            int align = get_align(ty);
            if (offset % align != 0) {
                offset += align - (offset % align);
            }
            ty->offset = offset;
            offset += size;
            push(type->fields, ty);
            if (consume("}")) break;
        }
    }
    return type;
}

// type.ty == INTを受け取りenumとみて初期化
// {は読んだとみる
Type *def_enum(Type *type) {
    if (type->ty != INT) {
        error_at(token->error, "enumではありません(コンパイラのバグ)");
    }
    int i = 0;
    if (!consume("}")) {
        while(!consume("}")) {
            Token *tok = consume_ident();
            if (strmapget(enumkeymap, tok->str, tok->len)) {
                error_at(tok->str, "enumとして定義済みです");
            }
            if (consume("=")) {
                i = calc(assign());
            }
            strmapset(enumkeymap, tok->str, tok->len, new_node_num(i++));
            if (!consume(",")) {
                expect("}");
                break;
            }
        }
    }
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
                    error_at(token->error, "関数を返す関数は利用できません");
                } else if (top_->ty == ARRAY) {
                    error_at(token->error, "関数の配列は利用できません");
                } else {
                    error_at(token->error, "unexpected error func");
                }
            }
            btm_->ty = FUNC;
            Type *arg;
            btm_->args = vec_new();
            if (!consume(")")) {
                // 引数を読む
                while(1) {
                    arg = eval_type_all();
                    if (!arg) error_at(token->error, "型として処理できません");
                    if (arg->ty == STRUCT) error_at(token->error, "関数の引数に構造体は渡せません(unimplemented)");
                    if (arg->ty == ARRAY) {
                        Type *tmp = arg;
                        arg = malloc(sizeof(Type));
                        memcpy(arg, tmp, sizeof(Type));
                        arg->ty = PTR; // 関数の引数ではint a[10]などはすべてポインタとする
                    }
                    push(btm_->args, arg);
                    if (arg->ty == VA_ARGS || !consume(",")) break;
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
                error_at(token->error, "配列を返す関数は利用できません");
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
            error_at(token->error, "unexpected error top_");
        }
        top = top_;
    }

    // tmp_top, tmp_btmとtop, btmを結合
    if (tmp_top) {
        if (top == NULL) {
            btm = tmp_btm;
        } else if (tmp_btm->ty == FUNC) {
            if (top->ty == ARRAY) {
                error_at(token->error, "配列を返す関数は利用できません");
            } else if (top->ty == FUNC) {
                error_at(token->error, "関数を返す関数は利用できません");
            }
            tmp_btm->ret = top;
        } else if (tmp_btm->ty == ARRAY) {
            if (top->ty == FUNC) {
                error_at(token->error, "関数の配列は利用できません");
            }
            tmp_btm->ptr_to = top;
        } else if (tmp_btm->ty == PTR) {
            tmp_btm->ptr_to = top;
        } else {
            error_at(token->error, "unexpected error tmp_top");
        }
        top = tmp_top;
    }

    // btm == NULLかつtop == NULLか、btm != NULLかつtop != NULLしかとらない(はず)
    *bottom = btm;
    return top;
}

// 先頭のトークンを呼んで、その型を返すかNULL
Type *eval_type_top() {
    Type *type = NULL;
    if(consumeTK(TK_INT)) {
        type = calloc(1, sizeof(Type));
        type->ty = INT;
    } else if (consumeTK(TK_CHAR)) {
        type = calloc(1, sizeof(Type));
        type->ty = CHAR;
    } else if (consumeTK(TK_SHORT)) {
        type = calloc(1, sizeof(Type));
        type->ty = SHORT;
    } else if (consume("...")) {
        type = calloc(1, sizeof(Type));
        type->ty = VA_ARGS;
    } else if (consumeTK(TK_VOID)) {
        type = calloc(1, sizeof(Type));
        type->ty = VOID;
    } else if (consumeTK(TK_STRUCT)) {
        Token *tok = consume_ident();
        if (tok) {
            type = strmapget(structmap, tok->str, tok->len);
            if (type != NULL) {
                if (consume("{")) {
                    def_struct(type);
                }
                return type;
            }
        }
        type = calloc(1, sizeof(Type));
        type->ty = STRUCT;
        if (tok) {
            strmapset(structmap, tok->str, tok->len, type);
        }
        if (consume("{")) {
            def_struct(type);
        }
    } else if (consumeTK(TK_ENUM)) {
        Token *tok = consume_ident();
        if (tok) {
            type = strmapget(enummap, tok->str, tok->len);
            if (type != NULL) {
                if (consume("{")) {
                    def_enum(type);
                }
                return type;
            }
        }
        type = calloc(1, sizeof(Type));
        type->ty = INT;
        if (consume("{")) {
            def_enum(type);
        }
        if (tok) {
            strmapset(enummap, tok->str, tok->len, type);
        }
    } else {
        Token *tok = token;
        if (tok->kind == TK_IDENT) {
            type = strmapget(typenamemap, tok->str, tok->len);
            if (type) {
                token = tok->next;
            }
        }
    }
    return type;
}

// 先頭の型(int, char, etc...)を受け取り、残りの型を処理する
Type *eval_type_ident(Type *type) {
    Token *tok = NULL;
    Type *btm = NULL;
    Type *top = eval_type_acc(&tok, &btm);
    if (btm) {
        if (btm->ty == FUNC) {
            btm->ret = type;
        } else if (btm->ty == ARRAY || btm->ty == PTR) {
            btm->ptr_to = type;
        } else {
            error_at(token->error, "unexpected error all btm, %d\n", btm->ty);
        }
    } else {
        type->tok = tok;
        return type;
    }
    top->tok = tok;
    return top;
}

// 完全な型を返す
Type *eval_type_all() {
    Type *type = eval_type_top();
    if (type)
        return eval_type_ident(type);
    else
        return NULL;
}

void show_type(Type *, int);

int type_equal(Type *t1, Type *t2) {
    if (t1 == t2) {
        return 1;
    }
    if (t1 == NULL || t2 == NULL) {
        return 0;
    }
    if (t1->ty == t2->ty) {
        if (t1->ty == PTR) {
            return type_equal(t1->ptr_to, t2->ptr_to);
        } else if (t1->ty == ARRAY) {
            return t1->array_size == t2->array_size && type_equal(t1->ptr_to, t2->ptr_to);
        } else if (t1->ty == FUNC) {
            Type *ty1;
            Type *ty2;

            // どちらかの最後が...なら返り値のみ検証する
            // TODO: ...の直前まで確認する
            if (t1->args->len > 1) {
                ty1 = t1->args->data[t1->args->len - 1];
                if (ty1->ty == VA_ARGS) {
                    return type_equal(t1->ret, t2->ret);
                }
            }
            if (t2->args->len > 1) {
                ty2 = t2->args->data[t2->args->len - 1];
                if (t2->ty == VA_ARGS) {
                    return type_equal(t1->ret, t2->ret);
                }
            }

            if (t1->args->len != t2->args->len) {
                return 0;
            } else {
                for (int i = 0; i < t1->args->len; i++) {
                    ty1 = t1->args->data[i];
                    ty2 = t2->args->data[i];
                    if (!type_equal(ty1, ty2)) {
                        return 0;
                    }
                }
                return type_equal(t1->ret, t2->ret);
            }
        } else if (t1->ty == STRUCT) {
            if (t1->fields->len != t2->fields->len) {
                return 0;
            }
            for (int i = 0; i < t1->args->len; i++) {
                Type *ty1 = t1->fields->data[i];
                Type *ty2 = t2->fields->data[i];
                if (!type_equal(ty1, ty2)) {
                    return 0;
                }
            }
            return 1;
        }
        return 1;
    }
    return 0;
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
        error_at(token->error, "assign lvarにva argsが入力されました(unimplemented)");
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


// extern宣言されていた場合そのまま返す
GVar *init_gvar(Type *type, Token *tok, int is_extern) {
    GVar *gvar = find_gvar(tok);
    if (gvar) {
        if (gvar->is_extern) {
            if (type_equal(type, gvar->type)) {
                return gvar;
            }
            show_type(type, 0);
            show_type(gvar->type, 0);
            error_at(tok->error, "extern宣言と型が一致しません");
        }
        error_at(token->error, "duplicate");
    }
    gvar = calloc(1, sizeof(GVar));
    gvar->name = tok->str;
    gvar->len = tok->len;
    gvar->type = type;
    gvar->size = sizeof_parse(type);
    gvar->is_extern = is_extern;
    return gvar;
}

Node *lvar_node(LVar *lvar) {
    Node *node = calloc(1, sizeof(Node));
    node->kind = ND_LVAR;
    node->lvar = lvar;
    node->type = lvar->type;
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

int is_ptr_or_array(Node *node) {
    return node->type->ty == PTR || node->type->ty == ARRAY;
}

Node *add_node(Node *lhs, Node *rhs) {
    if (is_ptr_or_array(lhs) && is_ptr_or_array(rhs)) {
        error_at(token->error, "ptr + ptrはできません");
    }
    if (is_ptr_or_array(lhs)) {
        Type *type = calloc(1, sizeof(Type));
        type->ty = PTR;
        type->ptr_to = lhs->type->ptr_to;
        return new_node(ND_ADD, lhs, new_node(ND_MUL, rhs, new_node_num(sizeof_parse(lhs->type->ptr_to)), NULL), type);
    }
    if (is_ptr_or_array(rhs)) {
        Type *type = calloc(1, sizeof(Type));
        type->ty = PTR;
        type->ptr_to = rhs->type->ptr_to;
        return new_node(ND_ADD, rhs, new_node(ND_MUL, lhs, new_node_num(sizeof_parse(rhs->type->ptr_to)), NULL), type);
    }
    return new_node(ND_ADD, lhs, rhs, lhs->type);
}

Node *sub_node(Node *lhs, Node *rhs) {
    if(is_ptr_or_array(lhs) && is_ptr_or_array(rhs)) {
        return new_node(ND_SUB, lhs, rhs, &int_type);
    }
    if(is_ptr_or_array(lhs)) {
        Type* type = calloc(1, sizeof(Type));
        type->ty = PTR;
        type->ptr_to = lhs->type->ptr_to;
        return new_node(ND_SUB, lhs, new_node(ND_MUL, new_node_num(sizeof_parse(lhs->type->ptr_to)), rhs, NULL), type);
    }
    if(is_ptr_or_array(rhs)) {
        error_at(token->error, "int - ptrは未定義です");
    }
    return new_node(ND_SUB, lhs, rhs, &int_type);
}

Node *deref_node(Node *n) {
    if (!is_ptr_or_array(n)) {
        error_at(token->error, "derefできません");
    }
    return new_node(ND_DEREF, n, NULL, n->type->ptr_to);
}

Node *assign_node(Node *lhs, Node *rhs) {
    if (lhs->kind != ND_LVAR && lhs->kind != ND_DEREF && lhs->kind != ND_GVAR) {
        error_at(token->error, "左辺に代入できません");
    }
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
    "int", "ptr", "array", "char", "func", "...", "void", "struct"
};

void show_type(Type *type, int indent) {
    if (type == NULL) {
        fprintf(stderr, "%*sNULL\n", indent, "");
        fprintf(stderr, "\n");
        return;
    }
    if (indent > 100) {
        fprintf(stderr, "%*sTooLarge\n", indent, "");
        fprintf(stderr, "\n");
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
        Vec *t = type->args;
        for(int i = 0; i < t->len; i++) {
            show_type(t->data[i], indent + 4);
        }
        fprintf(stderr, "%*sreturn:\n", indent + 2, "");
        show_type(type->ret, indent + 4);
    } else if (type->ty == STRUCT) {
        if (type->fields) {
            fprintf(stderr, "%*sfield:\n", indent + 2, "");
            for(int i = 0; i < type->fields->len; i++) {
                fprintf(stderr, "%*soffset: %d\n", indent + 4, "", ((Type *)type->fields->data[i])->offset);
                show_type(type->fields->data[i], indent + 4);
            }
        }
    }

    if (!indent) {
        fprintf(stderr, "\n");
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
            error_at(token->error, "type parse error");
        }
    }

    exit(1);
}

void program() {
    GVar **tmp = &code;
    while(consume(";"));
    globalstmt(tmp);
    while(consume(";"));

    while(!at_eof()) {
        if (*tmp) {
            tmp = &(*tmp)->next;
        }
        globalstmt(tmp);
        while(consume(";"));
    }
}

void globalstmt(GVar **ptr) {
    if (consumeTK(TK_TYPEDEF)) {
        Type *type = eval_type_all();
        if (!type->tok) {
            error_at(token->error, "型名がありません");
        }
        if (strmapget(typenamemap, type->tok->str, type->tok->len)) {
            error_at(token->error, "定義済みです");
        }
        strmapset(typenamemap, type->tok->str, type->tok->len, type);
        expect(";");
        return;
    }
    if (consumeTK(TK_EXTERN)) {
        Type *type = eval_type_all();
        if (!type || !type->tok) {
            error_at(token->error, "定義がありません");
        }
        GVar *gvar = init_gvar(type, type->tok, 1);
        expect(";");
        *ptr = gvar;
        return;
    }
    int is_static = 0;
    if (consumeTK(TK_STATIC)) {
        is_static = 1;
    }
    Type *type = eval_type_top();
    if (consume(";")) {
        return;
    }
    if (!type) {
        error_at(token->error, "type read error");
    }
    type = eval_type_ident(type);
    Token *tok = type->tok;
    if (!type || !tok) error_at(token->error, "型が正しくありません");
    GVar *gvar = init_gvar(type, tok, 0);
    gvar->is_static = is_static;
    if (type->ty == FUNC) {
        if (type->ret->ty == STRUCT) {
            error_at(token->error, "structを返す関数は利用できません(unimplemented)");
        }
        if (consume(";")) {
            gvar->is_extern = 1;
            *ptr = gvar;
            return;
        }
        locals = NULL;
        max_offset = 0;

        Vec *args = type->args;
        LVar *lvar;
        int reg_arglen = args->len > 6 ? 6 : args->len;
        for (int i = args->len - 1; i >= 0; i--) {
            Type *arg = args->data[i];
            if (arg->ty == VA_ARGS) {
                if (reg_arglen < 6) {
                    reg_arglen = 6;
                }
                for (int j = 0; j < 7 - i; j++) {
                    lvar = calloc(1, sizeof(LVar));
                    lvar->next = locals;
                    locals = lvar;
                }
            } else {
                lvar = init_lvar(arg);
                locals = lvar;
            }
        }
        lvar = locals;
        for (int i = 0; i < args->len && lvar; i++) {
            if (i >= 6) {
                lvar->offset = (5 - i) * 8;
            } else {
                lvar->offset = (reg_arglen - i) * 8;
            }
            lvar = lvar->next;
        }
        if (gvar->is_extern) {
            gvar->is_extern = 0;
        } else {
            *ptr = gvar;
        }
        if (*token->str != '{') {
            error_at(token->error, "{ ではありません");
        }
        gvar->node = calc_node(stmt());
        gvar->locals = locals;
        gvar->offset = max_offset;
        return;
    } else {
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
                            error_at(token->error, "引数が多すぎます");
                        }
                    } else {
                        type->array_size = i;
                        gvar->size = sizeof_parse(type);
                    }
                    gvar->node = node;
                } else if (token->kind == TK_STR) {
                    char *p = token->str + 1;
                    node = calloc(1, sizeof(Node));
                    node->kind = ND_BLOCK;
                    now = node;
                    for(; *p != '\"'; i++) {
                        now->lhs = calloc(1, sizeof(Node));
                        now->lhs->kind = ND_NUM;
                        now->lhs->type = &char_type;
                        now->lhs->val = read_one_char(&p); // jp[i];
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
                            error_at(token->error, "文字列が長すぎます");
                        }
                    } else {
                        type->array_size = i;
                        gvar->size = sizeof_parse(type);
                    }
                    gvar->node = node;
                    token = token->next;
                } else {
                    error_at(token->error, "unimplemented");
                }
            } else if (gvar->type->ty == STRUCT) {
                expect("{");
                Node *node = NULL;
                while(1) {
                    node = block_node(assign(), node);
                    if (!consume(",")) break;
                }
                expect("}");
                gvar->node = node;
            } else {
                gvar->node = calloc(1, sizeof(Node));
                gvar->node->lhs = assign();
                gvar->node->kind = ND_BLOCK;
            }
        }
        if (type->array_size == -1) {
            error_at(token->error, "配列の初期化ができません");
        }
        expect(";");
        gvar->node = calc_node(gvar->node);
        if (gvar->is_extern) {
            gvar->is_extern = 0;
            gvar->is_globl = 1;
        } else {
            *ptr = gvar;
        }
    }
}

Node *stmt() {
    Node *node = NULL;
    if(consumeTK(TK_RETURN)) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_RETURN;
        if (!consume(";")) {
            node->lhs = assign();
            expect(";");
        }
    } else if (consumeTK(TK_IF)) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_IF;
        expect("(");
        node->lhs = assign();
        expect(")");
        node->rhs = stmt();
        if (consumeTK(TK_ELSE))
            node->rhs = new_node(ND_ELSE, node->rhs, stmt(), NULL);
    } else if (consumeTK(TK_WHILE)) {
        node = calloc(1, sizeof(Node));
        node->kind = ND_WHILE;
        expect("(");
        node->lhs = assign();
        expect(")");
        node->rhs = stmt();
    } else if (consumeTK(TK_FOR)) {
        LVar *now_locals = locals;
        node = new_node(ND_FOR, NULL, new_node(ND_FOR, NULL, new_node(ND_FOR, NULL, NULL, NULL), NULL), NULL);
        expect("(");
        if(!consume(";")) {
            node->lhs = expr();
            expect(";");
        }
        if(!consume(";")) {
            node->rhs->lhs = assign();
            expect(";");
        } else {
            node->rhs->lhs = new_node_num(1);
        }
        if(!consume(")")) {
            node->rhs->rhs->lhs = assign();
            expect(")");
        }
        node->rhs->rhs->rhs = stmt();
        if (locals && (locals->offset > max_offset)) {
            max_offset = locals->offset;
        }
        locals = now_locals;
    } else if (consume("{")) {
        node = block_node(NULL, NULL);
        LVar *now_locals = locals;
        StrMap *now_structmap = structmap;
        structmap = calloc(1, sizeof(StrMap));
        structmap->value = now_structmap;
        while(!consume("}")) {
            node = block_node(stmt(), node);
        }
        if (locals && (locals->offset > max_offset)) {
            max_offset = locals->offset;
        }
        structmap = structmap->value;
        locals = now_locals;
    } else if(consumeTK(TK_BREAK)) {
        node = new_node(ND_BREAK, NULL, NULL, NULL);
        expect(";");
    } else if(consumeTK(TK_CONTINUE)) {
        node = new_node(ND_CONTINUE, NULL, NULL, NULL);
        expect(";");
    } else if(consumeTK(TK_SWITCH)) {
        LVar *now_locals = locals;
        node = new_node(ND_SWITCH, NULL, NULL, NULL);
        expect("(");
        node->lhs = assign();
        expect(")");
        expect("{");
        Node **ptr = &node->rhs;
        Vec *v = vec_new();
        while(1) {
            if (consumeTK(TK_CASE)) {
                int c = calc(assign());
                expect(":");
                for (int i = 0; i < v->len; i++) {
                    if (v->data[i] == c) {
                        error_at(token->error, "このcaseの数値はすでに使われています");
                    }
                }
                push(v, c);
                *ptr = new_node(ND_CASE, new_node_num(c), NULL, NULL);
                ptr = &(*ptr)->rhs;
            } else if (consumeTK(TK_DEFAULT)) {
                expect(":");
                *ptr = new_node(ND_DEFAULT, NULL, NULL, NULL);
                ptr = &(*ptr)->rhs;
            } else if (consume("}")) {
                break;
            } else {
                *ptr = new_node(ND_SWITCH, stmt(), NULL, NULL);
                ptr = &(*ptr)->rhs;
            }
        }
        free(v->data);
        free(v);
        if (locals && (locals->offset > max_offset)) {
            max_offset = locals->offset;
        }
        locals = now_locals;
    } else if (consumeTK(TK_DO)) {
        node = new_node(ND_DO, NULL, stmt(), NULL);
        if (!consumeTK(TK_WHILE)) {
            error_at(token->error, "whileがありません");
        }
        expect("(");
        node->lhs = assign();
        expect(")");
        expect(";");
    } else if (consume(";")) {
        return node;
    } else {
        node = expr();
        expect(";");
    }
    return node;
}

// (int | char) indent ([number])? (= assign | )? | assign
Node *expr() {
    Type *top = eval_type_top();
    if (*token->str == ';') {
        return new_node(ND_DECLARATION, NULL, NULL, NULL);
    }
    Node *top_node = NULL;
    Type *type = NULL;
    do {
        if (top) {
            type = eval_type_ident(top);
        }
        if(type) {
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
                                    error_at(token->error, "配列が長すぎます");
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
                        for (; *p != '"'; i++) {
                            node = block_node(assign_node(deref_node(add_node(ptr_node, new_node_num(i))), new_node_num(read_one_char(&p))), node);
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
                        error_at(token->error, "unexpected token");
                    }
                } else {
                    node = new_node(ND_ASSIGN, lvar_node(lvar), assign(), lvar->type);
                }
            }
            if (type->array_size == -1) {
                error_at(token->error, "配列の初期化ができません");
            }
            locals = lvar;
            top_node = new_node(ND_DECLARATION, node, top_node, NULL);
        }
    } while (consume(","));
    if (top_node) {
        return top_node;
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
        if (is_ptr_or_array(node) || is_ptr_or_array(right)) {
            error_at(token->error, "ポインタの掛け算は未定義です");
        }
        return assign_node(node, new_node(ND_MUL, node, right, &int_type));
    }
    if(consume("/=")) {
        Node *right = assign();
        if (is_ptr_or_array(node) || is_ptr_or_array(right)) {
            error_at(token->error, "ポインタの割り算は未定義です");
        }
        return assign_node(node, new_node(ND_DIV, node, right, &int_type));
    }
    if(consume("%=")) {
        Node *right = assign();
        if (is_ptr_or_array(node) || is_ptr_or_array(right)) {
            error_at(token->error, "ポインタの余剰は未定義です");
        }
        return assign_node(node, new_node(ND_REMINDER, node, right, &int_type));
    }
    if(consume("<<=")) {
        Node *right = assign();
        if (is_ptr_or_array(node) || is_ptr_or_array(right)) {
            error_at(token->error, "ポインタのシフト計算は未定義です");
        }
        return assign_node(node, new_node(ND_LSHIFT, node, right, &int_type));
    }
    if(consume(">>=")) {
        Node *right = assign();
        if (is_ptr_or_array(node) || is_ptr_or_array(right)) {
            error_at(token->error, "ポインタのシフト計算は未定義です");
        }
        return assign_node(node, new_node(ND_RSHIFT, node, right, &int_type));
    }
    if(consume("&=")) {
        Node *right = assign();
        if (is_ptr_or_array(node) || is_ptr_or_array(right)) {
            error_at(token->error, "ポインタのAND計算は未定義です");
        }
        return assign_node(node, new_node(ND_BITAND, node, right, &int_type));
    }
    if(consume("|=")) {
        Node *right = assign();
        if (is_ptr_or_array(node) || is_ptr_or_array(right)) {
            error_at(token->error, "ポインタのOR計算は未定義です");
        }
        return assign_node(node, new_node(ND_BITOR, node, right, &int_type));
    }
    if(consume("^=")) {
        Node *right = assign();
        if (is_ptr_or_array(node) || is_ptr_or_array(right)) {
            error_at(token->error, "ポインタのXOR計算は未定義です");
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

// cast ( * cast | / cast | % cast )*
Node *mul() {
    Node *node = cast();
    Node *right;

    for(;;) {
        if (consume("*")) {
            right = cast();
            if (is_ptr_or_array(node) || is_ptr_or_array(right))
                error_at(token->error, "ptr * ptr is not defined");
            node = new_node(ND_MUL, node, right, &int_type);
        }
        else if (consume("/")) {
            right = cast();
            if (is_ptr_or_array(node) || is_ptr_or_array(right))
                error_at(token->error, "ptr / ptr is not defined");
            node = new_node(ND_DIV, node, right, &int_type);
        }
        else if (consume("%")) {
            right = cast();
            if (is_ptr_or_array(node) || is_ptr_or_array(right))
                error_at(token->error, "ptr %% ptr is not defined");
            node = new_node(ND_REMINDER, node, right, &int_type);
        }
        else
            return node;
    }
}

// (type)? cast | unary
Node *cast() {
    Node *node;
    Token *tmp = token;
    if (consume("(")) {
        Type *ty = eval_type_all();
        if (ty) {
            expect(")");
            node = cast();
            node->type = ty;
            return node;
        }
        token = tmp;
    }
    return unary();
}

// primary (("[" assign "]")* | "(" assign,* ")" | ++ | -- )  |  ("*" | "&" | + | - | ! | ~) unary  |  sizeof unary
Node *unary() {
    Node *node;
    Type *tmp;
    if (consume("+"))
        return unary();
    if (consume("-")) {
        node = unary();
        if (is_ptr_or_array(node))
            error_at(token->error, "-ptr is not defined");
        return new_node(ND_NEG, node, NULL, node->type);
    }
    if (consume("*")) {
        node = unary();
        if (!is_ptr_or_array(node))
            error_at(token->error, "this is not ptr");
        if (node->type->ptr_to->ty == FUNC)
            return node;
        return new_node(ND_DEREF, node, NULL, node->type->ptr_to);
    }
    if (consume("&")) {
        if (consume("&")) error_at(token->error, "&& is not usable");
        node = unary();
        if (node->gvar && node->gvar->type->ty == PTR && node->gvar->type->ptr_to->ty == FUNC) {
            return node;
        }
        tmp = calloc(1, sizeof(Type));
        tmp->ptr_to = node->type;
        tmp->ty = PTR;
        return new_node(ND_ADDR, node, NULL, tmp);
    }
    if (consumeTK(TK_SIZEOF)) {
        Type *ty;
        if(*token->str == '(') {
            Token *tok = token;
            token = token->next;
            ty = eval_type_all();
            if (ty) {
                Node *n;
                n = new_node_num(sizeof_parse(ty));
                expect(")");
                return n;
            }
            token = tok;
        }
        node = unary();
        return new_node_num(sizeof_parse(node->type));
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
            if (node->type->ty != PTR || node->type->ptr_to->ty != FUNC) {
                error_at(token->error, "関数ではありません");
            }
            Node *n = calloc(1, sizeof(Node));
            n->kind = ND_CALL;
            n->type = node->type->ptr_to->ret;
            if (node->lhs && node->lhs->kind == ND_GVAR) {
                node = node->lhs; // deref
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
        } else if (consume(".")) {
            if (node->type->ty != STRUCT) {
                error_at(token->error, ".は未定義です");
            }
            Token *tok = consume_ident();
            if (!tok) {
                error_at(token->error, "identがありません");
            }
            Type *ty;
            for(int i = 0; i < node->type->fields->len; i++) {
                ty = node->type->fields->data[i];
                if (ty->tok->len == tok->len && memcmp(ty->tok->str, tok->str, tok->len) == 0) {
                    Type *t = calloc(1, sizeof(Type));
                    t->ptr_to = ty;
                    t->ty = PTR;
                    node = new_node(ND_ADDR, node, NULL, t);
                    node = new_node(ND_ADD, node, new_node_num(ty->offset), t);
                    node = deref_node(node);
                    break;
                }
            }
            if (ty == NULL) {
                error_at(tok->str, "このフィールドはありません");
            }
        } else if (consume("->")) {
            if (!(is_ptr_or_array(node) && node->type->ptr_to->ty == STRUCT)) {
                error_at(token->error, "->は未定義です");
            }
            Token *tok = consume_ident();
            if (!tok) {
                error_at(token->error, "identがありません");
            }
            Type *ty;
            for(int i = 0; i < node->type->ptr_to->fields->len; i++) {
                ty = node->type->ptr_to->fields->data[i];
                if (ty->tok->len == tok->len && memcmp(ty->tok->str, tok->str, tok->len) == 0) {
                    Type *t = calloc(1, sizeof(Type));
                    t->ptr_to = ty;
                    t->ty = PTR;
                    node = new_node(ND_ADD, node, new_node_num(ty->offset), t);
                    node = deref_node(node);
                    break;
                }
            }
            if (ty == NULL) {
                error_at(tok->str, "このフィールドはありません");
            }
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

    if (token->kind == TK_ONE_CHAR) {
        char *tmp = token->str + 1;
        token = token->next;
        return new_node_num(read_one_char(&tmp));
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
                node->kind = ND_GVAR;
                node->gvar = gvar;
                node->type = gvar->type;
                if (gvar->type->ty == FUNC) { // 関数はアドレスをとる
                    Type *tmp = calloc(1, sizeof(Type));
                    tmp->ty = PTR;
                    tmp->ptr_to = gvar->type;
                    return new_node(ND_ADDR, node, NULL, tmp);
                }
                return node;
            } else {
                Node *n = strmapget(enumkeymap, tok->str, tok->len);
                if (n) {
                    return n;
                }
            }
        }
        error_at(tok->str, "undefined");
    }

    return new_node_num(expect_number());
}
