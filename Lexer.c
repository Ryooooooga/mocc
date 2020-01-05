#include "mocc.h"

struct Lexer {
    const char *filename;
    const char *text;
    size_t cursor;
};

Lexer *Lexer_new(const char *filename, const char *text) {
    assert(filename);
    assert(text);

    Lexer *l = malloc(sizeof(Lexer));
    l->filename = filename;
    l->text = text;
    l->cursor = 0;

    return l;
}

static char Lexer_peek(Lexer *l) {
    assert(l);

    return l->text[l->cursor];
}

static char Lexer_consume(Lexer *l) {
    assert(l);
    assert(Lexer_peek(l) != '\0');

    return l->text[l->cursor++];
}

Token *Lexer_read(Lexer *l) {
    assert(l);

    Token *t = malloc(sizeof(Token));
    t->kind = -1;
    t->text = NULL;

    char buffer[1024]; // TODO: Dynamic buffer
    size_t len = 0;

    while (true) {
        const char c = Lexer_peek(l);

        if (c == '\0') {
            t->kind = '\0';
            break;
        } else if (isspace(c)) {
            Lexer_consume(l);
            continue;
        } else if (isdigit(c)) {
            while (isdigit(Lexer_peek(l))) {
                buffer[len++] = Lexer_consume(l);
            }

            t->kind = TokenKind_number;
            break;
        } else {
            t->kind = Lexer_consume(l);
            break;
        }
    }

    buffer[len] = '\0';
    t->text = strdup(buffer);
    return t;
}
