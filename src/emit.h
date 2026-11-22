#pragma once

#include <stdint.h>

/* Code buffer. */
int emit_pos(void);
void emit8(int b);
void emit32(uint32_t v);
void emit64(uint64_t v);
