#include "mocc.h"

typedef struct Preprocessor {
    Vec(Token) * result;
    Vec(Token) * queue;
    const char *filename;
    Lexer *lexer;
} Preprocessor;

static void
Preprocessor_init(Preprocessor *pp, const char *filename, const char *text) {
    assert(pp);
    assert(filename);
    assert(text);

    pp->result = Vec_new(Token)();
    pp->queue = Vec_new(Token)();
    pp->filename = filename;
    pp->lexer = Lexer_new(filename, text);
}

static void Preprocessor_expand_kw(Preprocessor *pp, Token *t) {
    assert(pp);
    assert(t);

    (void)pp;

    if (t->kind != TokenKind_identifier) {
        return;
    }

#define TOKEN_KW(name, text_)                                                  \
    if (strcmp(t->text, text_) == 0) {                                         \
        t->kind = TokenKind_##name;                                            \
        return;                                                                \
    }
#include "Token.def"
}

static Token *Preprocessor_current(Preprocessor *pp) {
    assert(pp);

    if (Vec_len(Token)(pp->queue) == 0) {
        Vec_push(Token)(pp->queue, Lexer_read(pp->lexer));
    }

    return Vec_get(Token)(pp->queue, 0);
}

static Token *Preprocessor_consume(Preprocessor *pp) {
    assert(pp);

    Token *front = Preprocessor_current(pp);

    for (size_t i = 1; i < Vec_len(Token)(pp->queue); i++) {
        Token *t = Vec_get(Token)(pp->queue, i);
        Vec_set(Token)(pp->queue, i - 1, t);
    }

    Vec_pop(Token)(pp->queue);
    return front;
}

static void Preprocessor_parse_directive(Preprocessor *pp) {
    assert(pp);
    assert(Preprocessor_current(pp)->kind == '#');

    // '#'
    Preprocessor_consume(pp);

    Token *directive = Preprocessor_current(pp);

    if (directive->is_bol || directive->kind == '\0') {
        return;
    } else {
        ERROR("unknown preprocessor directive %s\n", directive->text);
    }
}

Vec(Token) * Preprocessor_read(const char *filename, const char *text) {
    assert(filename);
    assert(text);

    Preprocessor pp;
    Preprocessor_init(&pp, filename, text);

    Token *t;
    do {
        t = Preprocessor_current(&pp);
        if (t->kind == '#' && t->is_bol) {
            // Preprocessor directive
            Preprocessor_parse_directive(&pp);
        } else {
            t = Preprocessor_consume(&pp);
            Preprocessor_expand_kw(&pp, t);
            Vec_push(Token)(pp.result, t);
        }
    } while (t->kind != '\0');

    return pp.result;
}
