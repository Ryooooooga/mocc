#include "mocc.h"

typedef struct {
    TokenKind kind;
    const char *text;
} TestToken;

static void check_lexer(
    const char *test_name,
    const char *text,
    const TestToken expected_tokens[]) {
    assert(test_name);
    assert(text);
    assert(expected_tokens);

    Lexer *l = Lexer_new(test_name, text);
    if (l == NULL) {
        ERROR("%s: Lexer_new() == NULL\n", test_name);
    }

    Token *t;
    int i = 0;
    do {
        t = Lexer_read(l);
        if (t == NULL) {
            ERROR("%s: Lexer_read() == NULL\n", test_name);
        }
        if (t->kind != expected_tokens[i].kind) {
            ERROR(
                "%s[%d]: t->kind != %d, actual %d\n",
                test_name,
                i,
                expected_tokens[i].kind,
                t->kind);
        }
        if (expected_tokens[i].text != NULL &&
            strcmp(t->text, expected_tokens[i].text) != 0) {
            ERROR(
                "%s[%d]: t->text != %s, actual %s\n",
                test_name,
                i,
                expected_tokens[i].text,
                t->text);
        }
    } while (expected_tokens[i++].kind != '\0');
}

void test_Lexer(void) {
    check_lexer(
        "empty",
        "",
        (TestToken[]){
            {.kind = '\0', NULL},
        });

    check_lexer(
        "number",
        "0 1 2 42 100",
        (TestToken[]){
            {.kind = TokenKind_number, "0"},
            {.kind = TokenKind_number, "1"},
            {.kind = TokenKind_number, "2"},
            {.kind = TokenKind_number, "42"},
            {.kind = TokenKind_number, "100"},
            {.kind = '\0', NULL},
        });

    check_lexer(
        "operators",
        "+ -",
        (TestToken[]){
            {.kind = '+', NULL},
            {.kind = '-', NULL},
            {.kind = '\0', NULL},
        });
}
