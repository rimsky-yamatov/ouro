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
