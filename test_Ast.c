#include "mocc.h"

static void
check_dump(const char *test_name, const Node *p, const char *expected) {
    assert(test_name);
    assert(expected);

    char path[256];
    snprintf(path, 256, "tmp/%s.txt", test_name);

    FILE *fp = fopen(path, "w");
    if (fp == NULL) {
        ERROR("could not create file %s", path);
    }

    Node_dump(p, fp);
    fclose(fp);

    char *actual = File_read(path);
    if (actual == NULL) {
        ERROR("could not read file %s", path);
    }

    remove(path);

    if (strcmp(actual, expected) != 0) {
        ERROR(
            "%s: actual != expected\n"
            "expected:\n"
            "---\n"
            "%s\n"
            "---\n"
            "actual:\n"
            "---\n"
            "%s\n"
            "---\n",
            test_name,
            expected,
            actual);
    }
}

void test_Ast(void) {
    check_dump("null", NULL, "(null)\n");

    check_dump(
        "integer_expr1",
        IntegerExprNode_cbase_node(IntegerExprNode_new(0)),
        "(IntegerExpr\n"
        "  (int 0)\n"
        ")\n");

    check_dump(
        "integer_expr2",
        IntegerExprNode_cbase_node(IntegerExprNode_new(42)),
        "(IntegerExpr\n"
        "  (int 42)\n"
        ")\n");

    check_dump(
        "return_stmt1",
        ReturnStmtNode_cbase_node(ReturnStmtNode_new(NULL)),
        "(ReturnStmt\n"
        "  (null)\n"
        ")\n");

    check_dump(
        "return_stmt2s",
        ReturnStmtNode_cbase_node(
            ReturnStmtNode_new(IntegerExprNode_base(IntegerExprNode_new(42)))),
        "(ReturnStmt\n"
        "  (IntegerExpr\n"
        "    (int 42)\n"
        "  )\n"
        ")\n");
}
