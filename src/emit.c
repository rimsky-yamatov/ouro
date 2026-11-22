#include "emit.h"

#include <string.h>

#include "lexer.h"
#include "ouro_limits.h"
#include "util.h"

static uint8_t code[MAX_OUTPUT];
static int code_len;

int emit_pos(void)
{
    return code_len;
}

void emit8(int b)
{
    if (code_len >= MAX_OUTPUT)
        die(tok_line, "output too large (max %d bytes)", MAX_OUTPUT);
    code[code_len++] = (uint8_t)b;
}

void emit32(uint32_t v)
{
    int i;

    for (i = 0; i < 4; i++)
        emit8((int)((v >> (8 * i)) & 0xFF));
}

void emit64(uint64_t v)
{
    int i;

    for (i = 0; i < 8; i++)
        emit8((int)((v >> (8 * i)) & 0xFF));
}

static int hex_digit(int c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    return -1;
}

void emit_hex(const char *s)
{
    while (*s) {
        int hi, lo;

        if (*s == ' ') {
            s++;
            continue;
        }
        hi = hex_digit(s[0]);
        lo = hex_digit(s[1]);
        if (hi < 0 || lo < 0)
            die(tok_line, "internal error: bad machine code string");
        emit8(hi * 16 + lo);
        s += 2;
    }
}
