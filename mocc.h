#ifndef INCLUDE_mocc_h
#define INCLUDE_mocc_h

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Macros
#define UNIMPLEMENTED() assert(!"unimplemented")
#define TODO(s) assert(!"todo: " s)

// Token
typedef int TokenKind;

enum {
    TokenKind_identifier = 256,
    TokenKind_number,
};

typedef struct Token {
    TokenKind kind;
    char *text; // For identifer and number
} Token;

// Lexer
typedef struct Lexer Lexer;

Lexer *Lexer_new(const char *filename, const char *text);
Token *Lexer_read(Lexer *l);

// Tests
#define ERROR(...) (fprintf(stderr, __VA_ARGS__), exit(1))

void test_Lexer(void);

#endif // INCLUDE_mocc_h
