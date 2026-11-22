#include "link.h"

#include <stdint.h>
#include <string.h>

#include "emit.h"
#include "lexer.h"
#include "ouro_limits.h"
#include "symtab.h"
#include "util.h"

#define LOAD_BASE   0x400000
#define ELF_EHDR    64
#define ELF_PHDR    56
#define HDR_SIZE    (ELF_EHDR + ELF_PHDR)

static void put16(uint8_t *p, unsigned v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}

static void put32(uint8_t *p, uint32_t v)
{
    put16(p, v & 0xFFFF);
    put16(p + 2, v >> 16);
}

static void put64(uint8_t *p, uint64_t v)
{
    put32(p, (uint32_t)v);
    put32(p + 4, (uint32_t)(v >> 32));
}

/* One PT_LOAD segment (RWX) covering the whole file; memsz > filesz leaves
   room for zero-initialized globals. */
static void build_header(uint8_t *h, int64_t filesz, int64_t memsz)
{
    memset(h, 0, HDR_SIZE);

    /* ELF header */
    h[0] = 0x7F; h[1] = 'E'; h[2] = 'L'; h[3] = 'F';
    h[4] = 2;                               /* 64-bit */
    h[5] = 1;                               /* little endian */
    h[6] = 1;                               /* ELF version */
    put16(h + 16, 2);                       /* ET_EXEC */
    put16(h + 18, 0x3E);                    /* x86-64 */
    put32(h + 20, 1);
    put64(h + 24, LOAD_BASE + HDR_SIZE);    /* entry: start of the code */
    put64(h + 32, ELF_EHDR);                /* program header offset */
    put16(h + 52, ELF_EHDR);
    put16(h + 54, ELF_PHDR);
    put16(h + 56, 1);                       /* one program header */
    put16(h + 58, 64);                      /* section header entry size */

    /* Program header */
    put32(h + 64, 1);                       /* PT_LOAD */
    put32(h + 68, 7);                       /* R | W | X */
    put64(h + 72, 0);                       /* offset */
    put64(h + 80, LOAD_BASE);               /* vaddr */
    put64(h + 88, LOAD_BASE);               /* paddr */
    put64(h + 96, (uint64_t)filesz);
    put64(h + 104, (uint64_t)memsz);
    put64(h + 112, 0x1000);                 /* align */
}

void link_write(FILE *out)
{
    uint8_t hdr[HDR_SIZE];
    int offs[MAX_FUNCS];
    int64_t code = emit_pos();
    int64_t data = data_size();
    int64_t bss = bss_size();
    int64_t filesz = HDR_SIZE + code + data;
    int64_t bss_off = (filesz + 7) & ~(int64_t)7;
    int64_t memsz = bss ? bss_off + bss : filesz;
    int i;

    if (filesz > MAX_OUTPUT)
        die(tok_line, "output too large (max %d bytes)", MAX_OUTPUT);
    for (i = 0; i < nfuncs; i++)
        offs[i] = funcs[i].code_off;
    emit_relocate(LOAD_BASE + HDR_SIZE + code, LOAD_BASE + bss_off, offs);
    build_header(hdr, filesz, memsz);

    fwrite(hdr, 1, HDR_SIZE, out);
    fwrite(code_bytes(), 1, (size_t)code, out);
    fwrite(data_bytes(), 1, (size_t)data, out);
}
