#include "mocc.h"

static void
check_parser(const char *test_name, const char *text, const char *expected) {
    assert(test_name);
    assert(text);
    assert(expected);

    Vec(Token) *tokens = Preprocessor_read(test_name, text);

    Parser *p = Parser_new(tokens);
    if (p == NULL) {
        ERROR("%s: Parser_new() == NULL\n", test_name);
    }

    TranslationUnitNode *node = Parser_parse(p);
    if (node == NULL) {
        ERROR("%s: Parser_parse() == NULL\n", test_name);
    }

    check_Node_dump(test_name, TranslationUnitNode_cbase_node(node), expected);
}

void test_Parser(void) {
    check_parser(
        "parser_minimum",
        "int main(void) {\n"
        "  return 0;\n"
        "}\n",
        "(TranslationUnit\n"
        "  (FunctionDecl\n"
        "    (DeclSpec\n"
        "    )\n"
        "    (FunctionDeclarator\n"
        "      (DirectDeclarator\n"
        "        (symbol main)\n"
        "      )\n"
        "    )\n"
        "    (CompoundStmt\n"
        "      (ReturnStmt\n"
        "        (IntegerExpr\n"
        "          (int 0)\n"
        "        )\n"
        "      )\n"
        "    )\n"
        "  )\n"
        ")\n");
}
