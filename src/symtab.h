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
