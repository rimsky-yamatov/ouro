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

static int hex_value(int c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

static void scan_hex(void)
{
    uint64_t v = 0;
    int digits = 0;

    pos += 2;                               /* 0x */
    while (hex_value(peekc(0)) >= 0) {
        if (v >> 60)
            die(line, "integer literal too large");
        v = (v << 4) | (uint64_t)hex_value(peekc(0));
        digits++;
        pos++;
    }
    if (digits == 0 || is_alpha(peekc(0)))
        die(line, "invalid hexadecimal literal");
    tok = T_INT;
    tok_int = (int64_t)v;
}

static void scan_decimal(void)
{
    uint64_t v = 0;

    while (is_digit(peekc(0))) {
        int d = peekc(0) - '0';

        if (v > (uint64_t)(INT64_MAX - d) / 10)
            die(line, "integer literal too large");
        v = v * 10 + (uint64_t)d;
        pos++;
    }
    if (is_alpha(peekc(0)))
        die(line, "invalid number");
    tok = T_INT;
    tok_int = (int64_t)v;
}

/* Decode the character after a backslash (already consumed). */
static int read_escape(void)
{
    int c = peekc(0);

    pos++;
    switch (c) {
    case 'n':  return '\n';
    case 't':  return '\t';
    case '0':  return 0;
    case '\\': return '\\';
    case '\'': return '\'';
    case '"':  return '"';
    default:
        die(line, "invalid escape sequence");
    }
    return 0;
}

static void scan_char(void)
{
    int c, v;

    pos++;                                  /* opening quote */
    c = peekc(0);
    if (c == -1 || c == '\n')
        die(line, "unterminated character literal");
    if (c == '\'')
        die(line, "empty character literal");
    if (c >= 128)
        die(line, "non-ASCII character in character literal");
    pos++;
    v = (c == '\\') ? read_escape() : c;
    if (peekc(0) != '\'')
        die(line, "unterminated character literal");
    pos++;
    tok = T_INT;
    tok_int = v;
}

static void scan_string(void)
{
    int n = 0;

    pos++;                                  /* opening quote */
    for (;;) {
        int c = peekc(0);

        if (c == -1 || c == '\n')
            die(line, "unterminated string literal");
        pos++;
        if (c == '"')
            break;
        if (c == '\\')
            c = read_escape();
        if (n >= MAX_STRDATA - 1)
            die(line, "string literal too long");
        tok_str[n++] = (char)c;
    }
    tok_str[n] = '\0';
    tok_str_len = n;
    tok = T_STR;
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
    if (is_digit(c)) {
        if (c == '0' && (peekc(1) == 'x' || peekc(1) == 'X'))
            scan_hex();
        else
            scan_decimal();
        return;
    }
    if (c == '\'') {
        scan_char();
        return;
    }
    if (c == '"') {
        scan_string();
        return;
    }
    die(line, "unexpected character");
}
