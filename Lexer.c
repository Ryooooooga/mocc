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

static char Lexer_current(Lexer *l) {
    assert(l);

    return l->text[l->cursor];
}

static char Lexer_consume(Lexer *l) {
    assert(l);
    assert(Lexer_current(l) != '\0');

    return l->text[l->cursor++];
}

Token *Lexer_read(Lexer *l) {
    assert(l);

    Token *t = malloc(sizeof(Token));
    t->kind = -1;
    t->text = NULL;
    t->string = NULL;
    t->string_len = 0;

    char buffer[1024]; // TODO: Dynamic buffer
    char str[1024];    // TODO: Dynamic buffer
    size_t len = 0;

    while (true) {
        const char c = Lexer_current(l);

        if (c == '\0') {
            t->kind = '\0';
            break;
        } else if (isspace(c)) {
            // \s
            Lexer_consume(l);
            continue;
        } else if (c == '\"') {
            // '\"'
            buffer[len++] = Lexer_consume(l);

            while (Lexer_current(l) != '\"') {
                if (Lexer_current(l) == '\0' || Lexer_current(l) == '\n') {
                    ERROR("unterminated string literal\n");
                }

                if (Lexer_current(l) == '\\') {
                    // '\\'
                    buffer[len++] = Lexer_consume(l);

                    switch ((buffer[len++] = Lexer_consume(l))) {
                    case '0':
                        str[t->string_len++] = '\0';
                        break;

                    case 'n':
                        str[t->string_len++] = '\n';
                        break;

                    default:
                        ERROR("unknown escape sequence\n");
                    }
                } else {
                    // .
                    str[t->string_len++] = buffer[len++] = Lexer_consume(l);
                }
            }

            // '\"'
            buffer[len++] = Lexer_consume(l);

            str[t->string_len++] = '\0';

            t->kind = TokenKind_string;
            t->string = strndup(str, t->string_len);
            break;
        } else if (isdigit(c)) {
            // [0-9]*
            while (isdigit(Lexer_current(l))) {
                buffer[len++] = Lexer_consume(l);
            }

            t->kind = TokenKind_number;
            break;
        } else if (isalpha(c)) {
            // [0-9A-Za-z]*
            while (isalnum(Lexer_current(l))) {
                buffer[len++] = Lexer_consume(l);
            }

            t->kind = TokenKind_identifier;
            break;
        } else if (c == '-') {
            buffer[len++] = Lexer_consume(l);

            if (Lexer_current(l) == '>') {
                buffer[len++] = Lexer_consume(l);
                t->kind = TokenKind_arrow;
            } else {
                t->kind = '-';
            }
            break;
        } else {
            // .
            t->kind = buffer[len++] = Lexer_consume(l);
            break;
        }
    }

    buffer[len] = '\0';
    t->text = strdup(buffer);
    return t;
}
