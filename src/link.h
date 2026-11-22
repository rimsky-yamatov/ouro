#pragma once

#include <stdio.h>

/* Lay out the program, resolve relocations and write the ELF image. */
void link_write(FILE *out);
