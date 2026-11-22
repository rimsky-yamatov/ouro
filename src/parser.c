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
