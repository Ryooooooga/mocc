#include "mocc.h"

typedef struct {
    TokenKind kind;
    const char *text;
    bool is_bol;
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
        if (strcmp(t->text, expected_tokens[i].text) != 0) {
            ERROR(
                "%s[%d]: t->text != %s, actual %s\n",
                test_name,
                i,
                expected_tokens[i].text,
                t->text);
        }
        if (t->is_bol != expected_tokens[i].is_bol) {
            ERROR(
                "%s[%d]: t->is_bol != %d, actual %d\n",
                test_name,
                i,
                expected_tokens[i].is_bol,
                t->is_bol);
        }
    } while (expected_tokens[i++].kind != '\0');
}

void test_Lexer(void) {
    check_lexer(
        "empty",
        "",
        (TestToken[]){
            {.kind = '\0', "", true},
        });

    check_lexer(
        "number",
        "0 1 2\n42 100",
        (TestToken[]){
            {.kind = TokenKind_number, "0", true},
            {.kind = TokenKind_number, "1", false},
            {.kind = TokenKind_number, "2", false},
            {.kind = TokenKind_number, "42", true},
            {.kind = TokenKind_number, "100", false},
            {.kind = '\0', "", true},
        });

    check_lexer(
        "identifier",
        "a bc Xyz\nif else if0",
        (TestToken[]){
            {.kind = TokenKind_identifier, "a", true},
            {.kind = TokenKind_identifier, "bc", false},
            {.kind = TokenKind_identifier, "Xyz", false},
            {.kind = TokenKind_identifier, "if", true},
            {.kind = TokenKind_identifier, "else", false},
            {.kind = TokenKind_identifier, "if0", false},
            {.kind = '\0', "", true},
        });

    check_lexer(
        "operators",
        "+ -",
        (TestToken[]){
            {.kind = '+', "+", true},
            {.kind = '-', "-", false},
            {.kind = '\0', "", true},
        });
}
