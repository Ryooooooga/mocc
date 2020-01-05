#include "mocc.h"

Vec(Token) * Preprocessor_read(const char *filename, const char *text) {
    assert(filename);
    assert(text);

    Vec(Token) *tokens = Vec_new(Token)();
    Lexer *l = Lexer_new(filename, text);

    Token *t;
    do {
        t = Lexer_read(l);
        Vec_push(Token)(tokens, t);
    } while (t->kind != '\0');

    return tokens;
}
