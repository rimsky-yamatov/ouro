#include "util.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void die(int line, const char *fmt, ...)
{
    va_list ap;

    fprintf(stderr, "line %d: ", line);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    exit(1);
}
