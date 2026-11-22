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
