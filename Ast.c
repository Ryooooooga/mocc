#include "mocc.h"

#define NODE(name)                                                             \
    static name##Node *name##Node_alloc(void) {                                \
        name##Node *p = malloc(sizeof(*p));                                    \
        p->kind = NodeKind_##name;                                             \
        return p;                                                              \
    }
#include "Ast.def"

// Expressions
IntegerExprNode *IntegerExprNode_new(long long value) {
    IntegerExprNode *p = IntegerExprNode_alloc();
    p->value = value;

    return p;
}

// Statements
CompoundStmtNode *CompoundStmtNode_new(void) {
    CompoundStmtNode *p = CompoundStmtNode_alloc();

    return p;
}

ReturnStmtNode *ReturnStmtNode_new(ExprNode *return_value) {
    ReturnStmtNode *p = ReturnStmtNode_alloc();
    p->return_value = return_value;

    return p;
}

// Declarations
FunctionDeclNode *FunctionDeclNode_new(void) {
    FunctionDeclNode *p = FunctionDeclNode_alloc();

    return p;
}

// Misc
TranslationUnitNode *TranslationUnitNode_new(void) {
    TranslationUnitNode *p = TranslationUnitNode_alloc();

    return p;
}

// Dump
static void Node_dump_impl(const Node *p, FILE *fp, size_t depth) {
    assert(fp);

    for (size_t i = 0; i < depth; i++) {
        fprintf(fp, "  ");
    }

    if (p == NULL) {
        fprintf(fp, "(null)\n");
        return;
    }

    UNIMPLEMENTED();
}

void Node_dump(const Node *p, FILE *fp) {
    assert(fp);

    Node_dump_impl(p, fp, 0);
}
