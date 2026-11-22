#pragma once

#include "ouro_limits.h"

/* Global variables and arrays. */
typedef struct {
    char name[MAX_IDENT + 1];
    int is_array;       /* 1: name evaluates to the address of the storage */
    int off;            /* offset in the global (bss) area */
} Global;

extern Global globals[MAX_GLOBALS];
extern int nglobals;

int global_find(const char *name);          /* index or -1 */
int global_add(const char *name, int is_array, int off, int line);

/* Functions, defined or only referenced so far by a call. */
typedef struct {
    char name[MAX_IDENT + 1];
    int nparams;
    int defined;
    int code_off;       /* valid once defined */
    int first_line;     /* line of the first reference or definition */
} Func;

extern Func funcs[MAX_FUNCS];
extern int nfuncs;

int func_find(const char *name);            /* index or -1 */
int func_add(const char *name, int nparams, int line);

/* Parameters and locals of the function being compiled. */
extern int nlocals;                         /* `var` locals, not parameters */

void locals_reset(void);
int local_find(const char *name, int *disp);    /* 1 if found; disp = rbp offset */
void param_add(const char *name, int index, int nparams);
void local_add(const char *name, int line);
