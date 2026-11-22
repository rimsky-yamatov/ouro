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

static IntVec data_fix;     /* positions of data address placeholders */
static IntVec bss_fix;      /* positions of bss address placeholders  */
static IntVec call_pos;     /* positions of call rel32 fields         */
static IntVec call_func;    /* function index for each call           */

void emit_data_addr32(int off)
{
    vec_push(&data_fix, code_len);
    emit32((uint32_t)off);
}

void emit_bss_addr32(int off)
{
    vec_push(&bss_fix, code_len);
    emit32((uint32_t)off);
}

void emit_call(int func)
{
    int p;

    emit8(0xE8);
    p = emit_pos();
    vec_push(&call_pos, p);
    vec_push(&call_func, func);
    emit32(0);
}

const uint8_t *code_bytes(void)
{
    return code;
}

const uint8_t *data_bytes(void)
{
    return data;
}

int data_size(void)
{
    return data_len;
}

int64_t bss_size(void)
{
    return bss_len;
}

static uint32_t get32(int pos)
{
    return (uint32_t)code[pos] | ((uint32_t)code[pos + 1] << 8) |
           ((uint32_t)code[pos + 2] << 16) | ((uint32_t)code[pos + 3] << 24);
}

void emit_relocate(int64_t data_base, int64_t bss_base, const int *func_off)
{
    int i;

    for (i = 0; i < data_fix.n; i++) {
        int p = data_fix.v[i];

        emit_patch32(p, get32(p) + (uint32_t)data_base);
    }
    for (i = 0; i < bss_fix.n; i++) {
        int p = bss_fix.v[i];

        emit_patch32(p, get32(p) + (uint32_t)bss_base);
    }
    for (i = 0; i < call_pos.n; i++) {
        int p = call_pos.v[i];

        emit_patch32(p, (uint32_t)(func_off[call_func.v[i]] - (p + 4)));
    }
}
