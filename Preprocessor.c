#include "mocc.h"

Vec(Token) * Preprocessor_read(const char *filename, const char *text) {
    assert(filename);
    assert(text);

    Vec(Token) *tokens = Vec_new(Token)();
    Lexer *l = Lexer_new(filename, text);

    Token *t;
    do {
        t = Lexer_read(l);

#define TOKEN_KW(name, text_)                                                  \
    if (t->kind == TokenKind_identifier && strcmp(t->text, text_) == 0) {      \
        t->kind = TokenKind_##name;                                            \
    }
#include "Token.def"

        Vec_push(Token)(tokens, t);
    } while (t->kind != '\0');

    return tokens;
}
