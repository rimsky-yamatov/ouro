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

void emit_patch32(int pos, uint32_t v)
{
    int i;

    for (i = 0; i < 4; i++)
        code[pos + i] = (uint8_t)((v >> (8 * i)) & 0xFF);
}

void patch_to(int pos, int target)
{
    emit_patch32(pos, (uint32_t)(target - (pos + 4)));
}

void patch_here(int pos)
{
    patch_to(pos, code_len);
}

int emit_jmp(void)
{
    int p;

    emit8(0xE9);
    p = emit_pos();
    emit32(0);
    return p;
}

int emit_jz(void)
{
    int p;

    emit_hex("0F 84");
    p = emit_pos();
    emit32(0);
    return p;
}

int emit_jnz(void)
{
    int p;

    emit_hex("0F 85");
    p = emit_pos();
    emit32(0);
    return p;
}

int emit_test_jz(void)
{
    emit_hex("48 85 C0");
    return emit_jz();
}

int emit_test_jnz(void)
{
    emit_hex("48 85 C0");
    return emit_jnz();
}

void emit_jmp_to(int target)
{
    int p = emit_jmp();

    patch_to(p, target);
}

static uint8_t data[MAX_STRDATA];
static int data_len;
static int64_t bss_len;

int data_add(const char *s, int len)
{
    int off = data_len;

    if (data_len + len + 1 > MAX_STRDATA)
        die(tok_line, "too many string literals (max %d bytes of string data)",
            MAX_STRDATA);
    memcpy(data + data_len, s, (size_t)len);
    data[data_len + len] = 0;
    data_len += len + 1;
    return off;
}

int bss_alloc(int64_t n)
{
    int off = (int)bss_len;

    n = (n + 7) & ~(int64_t)7;
    if (bss_len + n > MAX_BSS)
        die(tok_line, "too much global data (max %d bytes)", MAX_BSS);
    bss_len += n;
    return off;
}
