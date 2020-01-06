#include "mocc.h"

typedef struct {
    TokenKind kind;
    const char *text;
} TestToken;

static void check_pp(
    const char *test_name,
    const char *text,
    const TestToken expected_tokens[]) {
    assert(test_name);
    assert(text);
    assert(expected_tokens);

    Vec(Token) *tokens = Preprocessor_read(test_name, text);
    if (tokens == NULL) {
        ERROR("%s: Preprocessor_read() == NULL\n", test_name);
    }

    for (size_t i = 0; i < Vec_len(Token)(tokens); i++) {
        const Token *t = Vec_get(Token)(tokens, i);
        if (t == NULL) {
            ERROR("%s: tokens[%zu] == NULL\n", test_name, i);
        }
        if (t->kind != expected_tokens[i].kind) {
            ERROR(
                "%s: tokens[%zu]->kind != %d, actual %d\n",
                test_name,
                i,
                expected_tokens[i].kind,
                t->kind);
        }
        if (strcmp(t->text, expected_tokens[i].text) != 0) {
            ERROR(
                "%s: tokens[%zu]->text != %s, actual %s\n",
                test_name,
                i,
                expected_tokens[i].text,
                t->text);
        }
    }
}

void test_Preprocessor(void) {
    check_pp(
        "empty",
        "",
        (TestToken[]){
            {.kind = '\0', ""},
        });

    check_pp(
        "number",
        "0 1 2 42 100",
        (TestToken[]){
            {.kind = TokenKind_number, "0"},
            {.kind = TokenKind_number, "1"},
            {.kind = TokenKind_number, "2"},
            {.kind = TokenKind_number, "42"},
            {.kind = TokenKind_number, "100"},
            {.kind = '\0', ""},
        });

    check_pp(
        "identifier",
        "a bc Xyz if else if0",
        (TestToken[]){
            {.kind = TokenKind_identifier, "a"},
            {.kind = TokenKind_identifier, "bc"},
            {.kind = TokenKind_identifier, "Xyz"},
            {.kind = TokenKind_kw_if, "if"},
            {.kind = TokenKind_kw_else, "else"},
            {.kind = TokenKind_identifier, "if0"},
            {.kind = '\0', ""},
        });

    check_pp(
        "operators",
        "+ -",
        (TestToken[]){
            {.kind = '+', "+"},
            {.kind = '-', "-"},
            {.kind = '\0', ""},
        });
}
