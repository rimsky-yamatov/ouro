#pragma once

#include <stddef.h>
#include <stdint.h>

#include "ouro_limits.h"

typedef enum {
    T_EOF, T_INT, T_STR, T_IDENT,
    T_FN, T_VAR, T_IF, T_ELSE, T_WHILE, T_RETURN, T_BREAK,
    T_PLUS, T_MINUS, T_STAR, T_SLASH, T_PERCENT,
    T_AMP, T_PIPE, T_CARET, T_TILDE, T_BANG,
    T_SHL, T_SHR,
    T_EQ, T_NE, T_LT, T_LE, T_GT, T_GE,
    T_ANDAND, T_OROR, T_ASSIGN,
    T_LPAREN, T_RPAREN, T_LBRACE, T_RBRACE, T_LBRACK, T_RBRACK,
    T_COMMA, T_SEMI,
    T_COUNT
} Tok;

/* The current token and its attributes. */
extern Tok tok;
extern int tok_line;
extern int64_t tok_int;                 /* T_INT (character literals too) */
extern char tok_name[MAX_IDENT + 1];    /* T_IDENT */
extern char tok_str[MAX_STRDATA];       /* T_STR, decoded, NUL terminated */
extern int tok_str_len;                 /* T_STR, length without the NUL */

void lex_init(const char *src, size_t len);
void lex_next(void);

/* After an identifier token: is the next token a plain '=' (not '==')? */
int lex_peek_assign(void);

/* Display text of a token kind for diagnostics. */
const char *tok_text(Tok t);
