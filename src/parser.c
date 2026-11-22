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

/* name(args): the current token is '(' */
static void call(const char *name, int line)
{
    int argc = 0, b, f, disp;

    lex_next();
    if (tok != T_RPAREN) {
        for (;;) {
            if (argc >= MAX_PARAMS)
                die(tok_line, "too many arguments (max %d)", MAX_PARAMS);
            expr();
            emit8(0x50);                    /* push rax */
            argc++;
            if (tok != T_COMMA)
                break;
            lex_next();
        }
    }
    expect(T_RPAREN);

    b = builtin_find(name);
    if (b >= 0) {
        builtin_gen(b, argc, name, line);
        return;
    }
    f = func_find(name);
    if (f < 0) {
        if (local_find(name, &disp) || global_find(name) >= 0)
            die(line, "'%s' is not a function", name);
        f = func_add(name, argc, line);
    } else if (funcs[f].nparams != argc) {
        die(line, "function '%s' expects %d argument(s), got %d",
            name, funcs[f].nparams, argc);
    }
    emit_call(f);
    if (argc > 0) {
        emit_hex("48 83 C4");               /* add rsp, 8*argc */
        emit8(8 * argc);
    }
}

static void primary(void)
{
    switch (tok) {
    case T_INT:
        emit_const(tok_int);
        lex_next();
        break;
    case T_IDENT: {
        char name[MAX_IDENT + 1];
        int line = tok_line;

        strcpy(name, tok_name);
        lex_next();
        if (tok == T_LPAREN)
            call(name, line);
        else
            load_var(name, line);
        break;
    }
    case T_LPAREN:
        lex_next();
        expr();
        expect(T_RPAREN);
        break;
    case T_STR:
        emit8(0xB8);                        /* mov eax, imm32 (address) */
        emit_data_addr32(data_add(tok_str, tok_str_len));
        lex_next();
        break;
    default:
        die(tok_line, "unexpected %s in expression", tok_text(tok));
    }
}

static void unary(void)
{
    if (tok == T_MINUS) {
        lex_next();
        unary();
        emit_hex("48 F7 D8");               /* neg rax */
    } else if (tok == T_BANG) {
        lex_next();
        unary();
        emit_hex("48 85 C0 0F 94 C0 48 0F B6 C0");  /* rax = (rax == 0) */
    } else if (tok == T_TILDE) {
        lex_next();
        unary();
        emit_hex("48 F7 D0");               /* not rax */
    } else {
        primary();
    }
}

/* Binding strength of a binary operator; 0 if not one. Higher binds tighter. */
static int level_of(Tok t)
{
    switch (t) {
    case T_PLUS: case T_MINUS:
        return 9;
    case T_STAR: case T_SLASH: case T_PERCENT:
        return 10;
    default:
        return 0;
    }
}

/* rax = rax OP rcx */
static void apply_binop(Tok op)
{
    switch (op) {
    case T_PLUS:    emit_hex("48 01 C8"); break;
    case T_MINUS:   emit_hex("48 29 C8"); break;
    case T_STAR:    emit_hex("48 0F AF C1"); break;
    case T_SLASH:   emit_hex("48 99 48 F7 F9"); break;
    case T_PERCENT: emit_hex("48 99 48 F7 F9 48 89 D0"); break;
    default: break;
    }
}

/* Left-associative precedence climbing over levels >= minlvl. The left value
   is pushed while the right one is computed, then both are combined. */
static void binary(int minlvl)
{
    int lvl;

    unary();
    while ((lvl = level_of(tok)) >= minlvl) {
        Tok op = tok;

        lex_next();
        emit8(0x50);                        /* push rax */
        binary(lvl + 1);
        emit_hex("48 89 C1 58");            /* mov rcx, rax ; pop rax */
        apply_binop(op);
    }
}
