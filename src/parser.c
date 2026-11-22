/* Recursive-descent parser that emits x86-64 machine code directly.
 *
 * Every expression leaves its value in rax. Temporaries live on the stack.
 * Calls push their arguments left to right and the caller pops them. */
#include "parser.h"

#include <string.h>

#include "emit.h"
#include "lexer.h"
#include "ouro_limits.h"
#include "symtab.h"
#include "util.h"

#define CHAIN_MAX 256       /* operands in one && / || chain */

/* Pending 'break' jumps and, for each open loop, where its breaks start. */
static IntVec brk;
static IntVec loop_base;

static void expr(void);
static void stmt(void);
static void block(void);
static void unary(void);
static void primary(void);
static void binary(int minlvl);

static void expect(Tok t)
{
    if (tok != t)
        die(tok_line, "expected %s but found %s", tok_text(t), tok_text(tok));
    lex_next();
}

static void expect_ident(char *out)
{
    if (tok != T_IDENT)
        die(tok_line, "expected identifier but found %s", tok_text(tok));
    strcpy(out, tok_name);
    lex_next();
}

/* rax = constant */
static void emit_const(int64_t v)
{
    uint64_t u = (uint64_t)v;

    if (u <= 0xFFFFFFFFu) {
        emit8(0xB8);                        /* mov eax, imm32 */
        emit32((uint32_t)u);
    } else {
        emit_hex("48 B8");                  /* mov rax, imm64 */
        emit64(u);
    }
}

/* rax = value of a variable, or the address of a global array */
static void load_var(const char *name, int line)
{
    int disp, g;

    if (local_find(name, &disp)) {
        emit_hex("48 8B 85");               /* mov rax, [rbp+disp32] */
        emit32((uint32_t)disp);
        return;
    }
    g = global_find(name);
    if (g >= 0) {
        if (globals[g].is_array) {
            emit8(0xB8);                    /* mov eax, imm32 (address) */
        } else {
            emit_hex("48 8B 04 25");        /* mov rax, [abs32] */
        }
        emit_bss_addr32(globals[g].off);
        return;
    }
    if (func_find(name) >= 0 || builtin_find(name) >= 0)
        die(line, "'%s' is a function, not a variable", name);
    die(line, "undefined variable '%s'", name);
}

/* variable = rax */
static void store_var(const char *name, int line)
{
    int disp, g;

    if (local_find(name, &disp)) {
        emit_hex("48 89 85");               /* mov [rbp+disp32], rax */
        emit32((uint32_t)disp);
        return;
    }
    g = global_find(name);
    if (g >= 0) {
        if (globals[g].is_array)
            die(line, "cannot assign to array '%s'", name);
        emit_hex("48 89 04 25");            /* mov [abs32], rax */
        emit_bss_addr32(globals[g].off);
        return;
    }
    if (func_find(name) >= 0 || builtin_find(name) >= 0)
        die(line, "cannot assign to function '%s'", name);
    die(line, "undefined variable '%s'", name);
}

/* putc/eputc: the argument is on top of the stack; write its low byte. */
static void write_byte(int fd)
{
    emit8(0xBF);                            /* mov edi, fd */
    emit32((uint32_t)fd);
    emit_hex("48 89 E6");                   /* mov rsi, rsp */
    emit_hex("BA 01 00 00 00");             /* mov edx, 1 */
    emit_hex("B8 01 00 00 00");             /* mov eax, 1 (write) */
    emit_hex("0F 05");                      /* syscall */
    emit_hex("58");                         /* pop rax (drop the argument) */
    emit_hex("31 C0");                      /* xor eax, eax */
}

/* Built-ins are expanded inline. All arguments are already pushed. */
static void builtin_gen(int id, int argc, const char *name, int line)
{
    if (argc != builtins[id].nargs)
        die(line, "'%s' expects %d argument(s), got %d",
            name, builtins[id].nargs, argc);
    switch (id) {
    case B_PEEK:
        emit_hex("58 48 0F B6 00");         /* pop rax ; movzx rax, byte [rax] */
        break;
    case B_POKE:
        emit_hex("58 59 88 01 31 C0");      /* pop rax,rcx ; mov [rcx], al */
        break;
    case B_PEEKW:
        emit_hex("58 48 8B 00");            /* pop rax ; mov rax, [rax] */
        break;
    case B_POKEW:
        emit_hex("58 59 48 89 01 31 C0");   /* pop rax,rcx ; mov [rcx], rax */
        break;
    case B_GETC:
        /* read(0, rsp, 1) into a zeroed stack slot; -1 unless one byte came */
        emit_hex("6A 00 31 FF 48 89 E6 BA 01 00 00 00 31 C0 0F 05");
        emit_hex("48 83 F8 01 58 74 07 48 C7 C0 FF FF FF FF");
        break;
    case B_PUTC:
        write_byte(1);
        break;
    case B_EPUTC:
        write_byte(2);
        break;
    case B_EXIT:
        emit_hex("58 48 89 C7 B8 3C 00 00 00 0F 05");
        break;
    default:
        break;
    }
}
