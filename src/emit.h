#pragma once

#include <stdint.h>

/* Code buffer. */
int emit_pos(void);
void emit8(int b);
void emit32(uint32_t v);
void emit64(uint64_t v);

/* Emit raw bytes written as hex pairs, e.g. emit_hex("48 89 C1"). */
void emit_hex(const char *s);
