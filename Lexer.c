#include "mocc.h"

struct Lexer {
    const char *filename;
    const char *text;
    size_t cursor;
    bool is_bol;
};

Lexer *Lexer_new(const char *filename, const char *text) {
    assert(filename);
    assert(text);

    Lexer *l = malloc(sizeof(Lexer));
    l->filename = filename;
    l->text = text;
    l->cursor = 0;
    l->is_bol = true;

    return l;
}

static char Lexer_current(Lexer *l) {
    assert(l);

    return l->text[l->cursor];
}

static char Lexer_consume(Lexer *l) {
    assert(l);
    assert(Lexer_current(l) != '\0');

    l->cursor = l->cursor + 1;
    return l->text[l->cursor - 1];
}

static char Lexer_read_char(Lexer *l, char *buffer, size_t *len) {
    assert(l);

    if (Lexer_current(l) == '\0' || Lexer_current(l) == '\n') {
        ERROR("unterminated literal\n");
    }

    if (Lexer_current(l) == '\\') {
        // '\\'
        buffer[*len] = Lexer_consume(l);
        *len = *len + 1;

        char c = buffer[*len] = Lexer_consume(l);
        *len = *len + 1;
        if (c == '0') {
            return '\0';
        } else if (c == 'n') {
            return '\n';
        } else if (c == '\'') {
            return '\'';
        } else if (c == '\"') {
            return '\"';
        } else if (c == '\\') {
            return '\\';
        } else {
            ERROR("unknown escape sequence\n");
        }
    } else {
        // .
        char c = Lexer_consume(l);
        buffer[*len] = c;
        *len = *len + 1;
        return c;
    }
}

Token *Lexer_read(Lexer *l) {
    assert(l);

    Token *t = malloc(sizeof(Token));
    t->kind = -1;
    t->text = NULL;
    t->string = NULL;
    t->string_len = 0;
    t->is_bol = false;
    t->has_spaces = false;
    t->hidden_set = Vec_new(String)();

    char buffer[1024]; // TODO: Dynamic buffer
    char str[1024];    // TODO: Dynamic buffer
    size_t len = 0;

    bool ended = false;
    while (!ended) {
        const char c = Lexer_current(l);

        if (c == '\0') {
            l->is_bol = true;
            t->kind = '\0';
            ended = true;
        } else if (c == '\n') {
            // \s
            Lexer_consume(l);

            l->is_bol = true;
            t->has_spaces = true;
        } else if (isspace(c)) {
            // \s
            Lexer_consume(l);

            t->has_spaces = true;
        } else if (c == '\'') {
            // '\''
            buffer[len] = Lexer_consume(l);
            len = len + 1;

            while (Lexer_current(l) != '\'') {
                str[t->string_len] = Lexer_read_char(l, buffer, &len);
                t->string_len = t->string_len + 1;
            }

            // '\''
            buffer[len] = Lexer_consume(l);
            len = len + 1;

            t->kind = TokenKind_character;
            t->string = strndup(str, t->string_len);
            ended = true;
        } else if (c == '\"') {
            // '\"'
            buffer[len] = Lexer_consume(l);
            len = len + 1;

            while (Lexer_current(l) != '\"') {
                str[t->string_len] = Lexer_read_char(l, buffer, &len);
                t->string_len = t->string_len + 1;
            }

            // '\"'
            buffer[len] = Lexer_consume(l);
            len = len + 1;

            str[t->string_len] = '\0';
            t->string_len = t->string_len + 1;

            t->kind = TokenKind_string;
            t->string = strndup(str, t->string_len);
            ended = true;
        } else if (isdigit(c)) {
            // [0-9]*
            while (isdigit(Lexer_current(l))) {
                buffer[len] = Lexer_consume(l);
                len = len + 1;
            }

            t->kind = TokenKind_number;
            ended = true;
        } else if (isalpha(c) || c == '_') {
            // [0-9A-Za-z]*
            while (isalnum(Lexer_current(l)) || Lexer_current(l) == '_') {
                buffer[len] = Lexer_consume(l);
                len = len + 1;
            }

            t->kind = TokenKind_identifier;
            ended = true;
        } else if (c == '-') {
            buffer[len] = Lexer_consume(l);
            len = len + 1;

            if (Lexer_current(l) == '>') {
                buffer[len] = Lexer_consume(l);
                len = len + 1;
                t->kind = TokenKind_arrow;
            } else {
                t->kind = '-';
            }
            ended = true;
        } else if (c == '<') {
            buffer[len] = Lexer_consume(l);
            len = len + 1;

            if (Lexer_current(l) == '=') {
                buffer[len] = Lexer_consume(l);
                len = len + 1;
                t->kind = TokenKind_lesser_equal;
            } else {
                t->kind = '<';
            }
            ended = true;
        } else if (c == '>') {
            buffer[len] = Lexer_consume(l);
            len = len + 1;

            if (Lexer_current(l) == '=') {
                buffer[len] = Lexer_consume(l);
                len = len + 1;
                t->kind = TokenKind_greater_equal;
            } else {
                t->kind = '>';
            }
            ended = true;
        } else if (c == '=') {
            buffer[len] = Lexer_consume(l);
            len = len + 1;

            if (Lexer_current(l) == '=') {
                buffer[len] = Lexer_consume(l);
                len = len + 1;
                t->kind = TokenKind_equal;
            } else {
                t->kind = '=';
            }
            ended = true;
        } else if (c == '!') {
            buffer[len] = Lexer_consume(l);
            len = len + 1;

            if (Lexer_current(l) == '=') {
                buffer[len] = Lexer_consume(l);
                len = len + 1;
                t->kind = TokenKind_not_equal;
            } else {
                t->kind = '!';
            }
            ended = true;
        } else if (c == '&') {
            buffer[len] = Lexer_consume(l);
            len = len + 1;

            if (Lexer_current(l) == '&') {
                buffer[len] = Lexer_consume(l);
                len = len + 1;
                t->kind = TokenKind_and_and;
            } else {
                t->kind = '&';
            }
            ended = true;
        } else if (c == '|') {
            buffer[len] = Lexer_consume(l);
            len = len + 1;

            if (Lexer_current(l) == '|') {
                buffer[len] = Lexer_consume(l);
                len = len + 1;
                t->kind = TokenKind_or_or;
            } else {
                t->kind = '|';
            }
            ended = true;
        } else if (c == '.') {
            buffer[len] = Lexer_consume(l);
            len = len + 1;

            if (Lexer_current(l) == '.') {
                buffer[len] = Lexer_consume(l);
                len = len + 1;

                if (Lexer_current(l) == '.') {
                    buffer[len] = Lexer_consume(l);
                    len = len + 1;
                    t->kind = TokenKind_var_arg;
                } else {
                    t->kind = TokenKind_dot_dot;
                }
            } else {
                t->kind = '.';
            }
            ended = true;
        } else {
            // .
            t->kind = buffer[len] = Lexer_consume(l);
            len = len + 1;
            ended = true;
        }
    }

    buffer[len] = '\0';
    t->text = strdup(buffer);
    t->is_bol = l->is_bol;

    l->is_bol = false;
    return t;
}
