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

    Vec(String) *include_paths = Vec_new(String)();
    Vec_push(String)(include_paths, "test");

    Vec(Token) *tokens = Preprocessor_read(include_paths, test_name, text);
    if (tokens == NULL) {
        ERROR("%s: Preprocessor_read() == NULL\n", test_name);
    }

    size_t len = Vec_len(Token)(tokens);

    for (size_t i = 0; i < len; i++) {
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

    if (len == 0 || Vec_get(Token)(tokens, len - 1)->kind != '\0') {
        ERROR("%s: tokens must be ended with EOF\n", test_name);
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

    check_pp(
        "empty-directive",
        "#\n"
        "A B\n"
        "#\n"
        "C\n"
        "#",
        (TestToken[]){
            {.kind = TokenKind_identifier, "A"},
            {.kind = TokenKind_identifier, "B"},
            {.kind = TokenKind_identifier, "C"},
            {.kind = '\0', ""},
        });

    check_pp(
        "define-directive",
        "#define A (a b)\n"
        "#define B A c A\n"
        "#define C\n"
        "A B C",
        (TestToken[]){
            {.kind = '(', "("},
            {.kind = TokenKind_identifier, "a"},
            {.kind = TokenKind_identifier, "b"},
            {.kind = ')', ")"},
            {.kind = '(', "("},
            {.kind = TokenKind_identifier, "a"},
            {.kind = TokenKind_identifier, "b"},
            {.kind = ')', ")"},
            {.kind = TokenKind_identifier, "c"},
            {.kind = '(', "("},
            {.kind = TokenKind_identifier, "a"},
            {.kind = TokenKind_identifier, "b"},
            {.kind = ')', ")"},
            {.kind = '\0', ""},
        });

    check_pp(
        "define-recursive",
        "#define A A\n"
        "#define B C\n"
        "#define C B\n"
        "A B C",
        (TestToken[]){
            {.kind = TokenKind_identifier, "A"},
            {.kind = TokenKind_identifier, "B"},
            {.kind = TokenKind_identifier, "C"},
            {.kind = '\0', ""},
        });

    check_pp(
        "function-macro",
        "#define F() f(x)\n"
        "F F()",
        (TestToken[]){
            {.kind = TokenKind_identifier, "F"},
            {.kind = TokenKind_identifier, "f"},
            {.kind = '(', "("},
            {.kind = TokenKind_identifier, "x"},
            {.kind = ')', ")"},
            {.kind = '\0', ""},
        });

    check_pp(
        "include",
        "#include \"test.h\"\n"
        "DEFINED_IN_test_h",
        (TestToken[]){
            {.kind = TokenKind_kw_int, "int"},
            {.kind = TokenKind_identifier, "hello"},
            {.kind = '(', "("},
            {.kind = TokenKind_kw_void, "void"},
            {.kind = ')', ")"},
            {.kind = ';', ";"},
            {.kind = TokenKind_identifier, "defined"},
            {.kind = TokenKind_identifier, "in"},
            {.kind = TokenKind_identifier, "test_h"},
            {.kind = '\0', ""},
        });
}
