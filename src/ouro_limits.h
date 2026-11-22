#pragma once

/* Reference limits from the language specification (section 14). */
#define MAX_SOURCE   (256 * 1024)
#define MAX_IDENT    31
#define MAX_FUNCS    512
#define MAX_GLOBALS  512
#define MAX_LOCALS   256
#define MAX_PARAMS   8
#define MAX_STRDATA  (64 * 1024)
#define MAX_OUTPUT   (1024 * 1024)

/* Upper bound for global storage; keeps every address within a signed
   32-bit displacement. */
#define MAX_BSS      (1 << 30)
