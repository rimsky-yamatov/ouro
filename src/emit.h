#pragma once

#include <stdint.h>

/* Code buffer. */
int emit_pos(void);
void emit8(int b);
void emit32(uint32_t v);
void emit64(uint64_t v);

/* Emit raw bytes written as hex pairs, e.g. emit_hex("48 89 C1"). */
void emit_hex(const char *s);

/* Jumps use rel32. The emit_j* functions return the position of the rel32
   field so it can be patched once the target is known. */
int emit_jmp(void);
int emit_jz(void);
int emit_jnz(void);
int emit_test_jz(void);     /* test rax, rax ; jz  */
int emit_test_jnz(void);    /* test rax, rax ; jnz */
void emit_jmp_to(int target);
void emit_patch32(int pos, uint32_t v);
void patch_to(int pos, int target);
void patch_here(int pos);

/* String literal data and zero-initialized global storage. */
int data_add(const char *s, int len);       /* returns the offset */
int bss_alloc(int64_t n);                   /* returns the offset */

/* 32-bit placeholders holding a data/bss offset, relocated at link time. */
void emit_data_addr32(int off);
void emit_bss_addr32(int off);
void emit_call(int func);                   /* call rel32 to function index */

/* Accessors for the linker. */
const uint8_t *code_bytes(void);
const uint8_t *data_bytes(void);
int data_size(void);
int64_t bss_size(void);

/* Resolve all recorded relocations. func_off[i] is the code offset of
   function i. */
void emit_relocate(int64_t data_base, int64_t bss_base, const int *func_off);
