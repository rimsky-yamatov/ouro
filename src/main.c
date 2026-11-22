#define _POSIX_C_SOURCE 200809L

#include <stdio.h>

#include "ouro_limits.h"
#include "util.h"

static char srcbuf[MAX_SOURCE + 1];

int main(void)
{
    size_t n = fread(srcbuf, 1, MAX_SOURCE + 1, stdin);

    if (n > MAX_SOURCE)
        die(1, "source too large (max %d bytes)", MAX_SOURCE);
    return 0;
}
