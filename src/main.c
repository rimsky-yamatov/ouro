#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>

#include "lexer.h"
#include "link.h"
#include "ouro_limits.h"
#include "parser.h"
#include "util.h"

#ifndef OURO_VERSION
#define OURO_VERSION "0.1.0-dev"
#endif

static char srcbuf[MAX_SOURCE + 1];

static void usage(FILE *out)
{
    fputs("usage: ouroc0 < source.ou > program\n"
          "\n"
          "Compiles an Ouro program from standard input into an x86-64 Linux\n"
          "ELF executable on standard output.\n"
          "\n"
          "options:\n"
          "  --version       print the version and exit\n"
          "  -h, --help      print this help and exit\n"
          "  --dump-tokens   print the token stream (debugging)\n",
          out);
}

static void dump_tokens(void)
{
    for (lex_next(); tok != T_EOF; lex_next()) {
        printf("%d: %s", tok_line, tok_text(tok));
        if (tok == T_IDENT)
            printf(" %s", tok_name);
        else if (tok == T_INT)
            printf(" %lld", (long long)tok_int);
        else if (tok == T_STR)
            printf(" \"%s\"", tok_str);
        putchar('\n');
    }
}

int main(int argc, char **argv)
{
    int dump = 0;
    size_t n;

    if (argc > 1) {
        if (strcmp(argv[1], "--version") == 0) {
            printf("ouroc0 %s\n", OURO_VERSION);
            return 0;
        }
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            usage(stdout);
            return 0;
        }
        if (strcmp(argv[1], "--dump-tokens") == 0 && argc == 2) {
            dump = 1;
        } else {
            usage(stderr);
            return 2;
        }
    }

    n = fread(srcbuf, 1, MAX_SOURCE + 1, stdin);
    if (n > MAX_SOURCE)
        die(1, "source too large (max %d bytes)", MAX_SOURCE);
    lex_init(srcbuf, n);

    if (dump) {
        dump_tokens();
        return 0;
    }

    parse_program();
    link_write(stdout);
    if (fflush(stdout) != 0 || ferror(stdout)) {
        fputs("ouroc0: write error\n", stderr);
        return 1;
    }
    return 0;
}
