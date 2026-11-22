#include "lexer.h"

#include <string.h>

#include "util.h"

Tok tok;
int tok_line;
int64_t tok_int;
char tok_name[MAX_IDENT + 1];
char tok_str[MAX_STRDATA];
int tok_str_len;

static const char *src;
static size_t src_len;
static size_t pos;
static int line;

void lex_init(const char *s, size_t len)
{
    src = s;
    src_len = len;
    pos = 0;
    line = 1;
    tok = T_EOF;
    tok_line = 1;
}

/* Byte at pos + k, or -1 past the end of the source. */
static int peekc(size_t k)
{
    return pos + k < src_len ? (unsigned char)src[pos + k] : -1;
}

static int is_alpha(int c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static int is_digit(int c)
{
    return c >= '0' && c <= '9';
}

static const struct {
    const char *text;
    Tok tok;
} keywords[] = {
    { "fn", T_FN },       { "var", T_VAR },       { "if", T_IF },
    { "else", T_ELSE },   { "while", T_WHILE },   { "return", T_RETURN },
    { "break", T_BREAK },
};

static void scan_ident(void)
{
    size_t i;
    int n = 0;

    while (is_alpha(peekc(0)) || is_digit(peekc(0))) {
        if (n >= MAX_IDENT)
            die(line, "identifier too long (max %d characters)", MAX_IDENT);
        tok_name[n++] = (char)peekc(0);
        pos++;
    }
    tok_name[n] = '\0';
    tok = T_IDENT;
    for (i = 0; i < sizeof keywords / sizeof keywords[0]; i++) {
        if (strcmp(tok_name, keywords[i].text) == 0) {
            tok = keywords[i].tok;
            break;
        }
    }
}

static void skip_space(void)
{
    for (;;) {
        int c = peekc(0);

        if (c == ' ' || c == '\t' || c == '\r') {
            pos++;
        } else if (c == '\n') {
            line++;
            pos++;
        } else if (c == '#') {
            while (peekc(0) != -1 && peekc(0) != '\n')
                pos++;
        } else {
            break;
        }
    }
}

void lex_next(void)
{
    int c;

    skip_space();
    tok_line = line;
    c = peekc(0);
    if (c == -1) {
        tok = T_EOF;
        return;
    }
    if (is_alpha(c)) {
        scan_ident();
        return;
    }
    die(line, "unexpected character");
}
