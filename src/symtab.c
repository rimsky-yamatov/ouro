#include "symtab.h"

#include <string.h>

#include "util.h"

Global globals[MAX_GLOBALS];
int nglobals;

int global_find(const char *name)
{
    int i;

    for (i = 0; i < nglobals; i++)
        if (strcmp(globals[i].name, name) == 0)
            return i;
    return -1;
}

int global_add(const char *name, int is_array, int off, int line)
{
    Global *g;

    if (nglobals >= MAX_GLOBALS)
        die(line, "too many global variables (max %d)", MAX_GLOBALS);
    g = &globals[nglobals];
    strcpy(g->name, name);
    g->is_array = is_array;
    g->off = off;
    return nglobals++;
}

Func funcs[MAX_FUNCS];
int nfuncs;

int func_find(const char *name)
{
    int i;

    for (i = 0; i < nfuncs; i++)
        if (strcmp(funcs[i].name, name) == 0)
            return i;
    return -1;
}

int func_add(const char *name, int nparams, int line)
{
    Func *f;

    if (nfuncs >= MAX_FUNCS)
        die(line, "too many functions (max %d)", MAX_FUNCS);
    f = &funcs[nfuncs];
    strcpy(f->name, name);
    f->nparams = nparams;
    f->defined = 0;
    f->code_off = 0;
    f->first_line = line;
    return nfuncs++;
}

static char local_name[MAX_LOCALS + MAX_PARAMS][MAX_IDENT + 1];
static int local_disp[MAX_LOCALS + MAX_PARAMS];
static int nentries;
int nlocals;

void locals_reset(void)
{
    nentries = 0;
    nlocals = 0;
}

int local_find(const char *name, int *disp)
{
    int i;

    for (i = 0; i < nentries; i++) {
        if (strcmp(local_name[i], name) == 0) {
            *disp = local_disp[i];
            return 1;
        }
    }
    return 0;
}

/* Arguments are pushed left to right, so with n parameters the last one sits
   right above the saved rbp and return address. */
void param_add(const char *name, int index, int nparams)
{
    strcpy(local_name[nentries], name);
    local_disp[nentries] = 16 + 8 * (nparams - 1 - index);
    nentries++;
}

void local_add(const char *name, int line)
{
    if (nlocals >= MAX_LOCALS)
        die(line, "too many local variables (max %d)", MAX_LOCALS);
    strcpy(local_name[nentries], name);
    local_disp[nentries] = -8 * (nlocals + 1);
    nentries++;
    nlocals++;
}

const Builtin builtins[B_COUNT] = {
    { "peek", 1 },  { "poke", 2 },  { "peekw", 1 }, { "pokew", 2 },
    { "getc", 0 },  { "putc", 1 },  { "eputc", 1 }, { "exit", 1 },
};

int builtin_find(const char *name)
{
    int i;

    for (i = 0; i < B_COUNT; i++)
        if (strcmp(builtins[i].name, name) == 0)
            return i;
    return -1;
}
