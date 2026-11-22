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
