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
