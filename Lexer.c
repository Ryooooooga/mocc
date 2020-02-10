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

    return l->text[l->cursor++];
}

static char Lexer_read_char(Lexer *l, char *buffer, size_t *len) {
    assert(l);

    if (Lexer_current(l) == '\0' || Lexer_current(l) == '\n') {
        ERROR("unterminated literal\n");
    }

    if (Lexer_current(l) == '\\') {
        // '\\'
        buffer[(*len)++] = Lexer_consume(l);

        switch ((buffer[(*len)++] = Lexer_consume(l))) {
        case '0':
            return '\0';

        case 'n':
            return '\n';

        default:
            ERROR("unknown escape sequence\n");
        }
    } else {
        // .
        return buffer[(*len)++] = Lexer_consume(l);
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

    while (true) {
        const char c = Lexer_current(l);

        if (c == '\0') {
            l->is_bol = true;
            t->kind = '\0';
            break;
        } else if (c == '\n') {
            // \s
            Lexer_consume(l);

            l->is_bol = true;
            t->has_spaces = true;
            continue;
        } else if (isspace(c)) {
            // \s
            Lexer_consume(l);

            t->has_spaces = true;
            continue;
        } else if (c == '\'') {
            // '\''
            buffer[len++] = Lexer_consume(l);

            while (Lexer_current(l) != '\'') {
                str[t->string_len++] = Lexer_read_char(l, buffer, &len);
            }

            // '\''
            buffer[len++] = Lexer_consume(l);

            t->kind = TokenKind_character;
            t->string = strndup(str, t->string_len);
            break;
        } else if (c == '\"') {
            // '\"'
            buffer[len++] = Lexer_consume(l);

            while (Lexer_current(l) != '\"') {
                str[t->string_len++] = Lexer_read_char(l, buffer, &len);
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
        } else if (isalpha(c) || c == '_') {
            // [0-9A-Za-z]*
            while (isalnum(Lexer_current(l)) || Lexer_current(l) == '_') {
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
        } else if (c == '<') {
            buffer[len++] = Lexer_consume(l);

            if (Lexer_current(l) == '=') {
                buffer[len++] = Lexer_consume(l);
                t->kind = TokenKind_lesser_equal;
            } else {
                t->kind = '<';
            }
            break;
        } else if (c == '>') {
            buffer[len++] = Lexer_consume(l);

            if (Lexer_current(l) == '=') {
                buffer[len++] = Lexer_consume(l);
                t->kind = TokenKind_greater_equal;
            } else {
                t->kind = '>';
            }
            break;
        } else if (c == '=') {
            buffer[len++] = Lexer_consume(l);

            if (Lexer_current(l) == '=') {
                buffer[len++] = Lexer_consume(l);
                t->kind = TokenKind_equal;
            } else {
                t->kind = '=';
            }
            break;
        } else if (c == '!') {
            buffer[len++] = Lexer_consume(l);

            if (Lexer_current(l) == '=') {
                buffer[len++] = Lexer_consume(l);
                t->kind = TokenKind_not_equal;
            } else {
                t->kind = '!';
            }
            break;
        } else if (c == '&') {
            buffer[len++] = Lexer_consume(l);

            if (Lexer_current(l) == '&') {
                buffer[len++] = Lexer_consume(l);
                t->kind = TokenKind_and_and;
            } else {
                t->kind = '&';
            }
            break;
        } else if (c == '|') {
            buffer[len++] = Lexer_consume(l);

            if (Lexer_current(l) == '|') {
                buffer[len++] = Lexer_consume(l);
                t->kind = TokenKind_or_or;
            } else {
                t->kind = '|';
            }
            break;
        } else if (c == '.') {
            buffer[len++] = Lexer_consume(l);

            if (Lexer_current(l) == '.') {
                buffer[len++] = Lexer_consume(l);

                if (Lexer_current(l) == '.') {
                    buffer[len++] = Lexer_consume(l);
                    t->kind = TokenKind_var_arg;
                } else {
                    t->kind = TokenKind_dot_dot;
                }
            } else {
                t->kind = '.';
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
    t->is_bol = l->is_bol;

    l->is_bol = false;
    return t;
}
