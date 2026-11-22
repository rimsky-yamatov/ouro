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

void vec_push(IntVec *s, int x)
{
    if (s->n == s->cap) {
        int ncap = s->cap ? s->cap * 2 : 64;
        int *nv = (int *)realloc(s->v, (size_t)ncap * sizeof(int));
        if (!nv)
            die(0, "out of memory");
        s->v = nv;
        s->cap = ncap;
    }
    s->v[s->n++] = x;
}

int vec_pop(IntVec *s)
{
    return s->v[--s->n];
}
