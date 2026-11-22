#pragma once

#if defined(_MSC_VER)
#define OURO_NORETURN __declspec(noreturn)
#else
#define OURO_NORETURN __attribute__((noreturn))
#endif

#if defined(__GNUC__) || defined(__clang__)
#define OURO_PRINTF(f, a) __attribute__((format(printf, f, a)))
#else
#define OURO_PRINTF(f, a)
#endif

/* Print "line N: message" to stderr and exit with status 1. */
OURO_NORETURN OURO_PRINTF(2, 3) void die(int line, const char *fmt, ...);
