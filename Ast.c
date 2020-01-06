#include "mocc.h"

#define NODE(name, base)                                                       \
    static name##base##Node *name##base##Node_alloc(void) {                    \
        name##base##Node *p = malloc(sizeof(*p));                              \
        p->kind = NodeKind_##name##base;                                       \
        return p;                                                              \
    }                                                                          \
                                                                               \
    Node *name##base##Node_base_node(name##base##Node *p) {                    \
        assert(p);                                                             \
        return (Node *)p;                                                      \
    }                                                                          \
    const Node *name##base##Node_cbase_node(name##base##Node *p) {             \
        assert(p);                                                             \
        return (const Node *)p;                                                \
    }                                                                          \
    base##Node *name##base##Node_base(name##base##Node *p) {                   \
        assert(p);                                                             \
        return (base##Node *)p;                                                \
    }                                                                          \
    const base##Node *name##base##Node_cbase(const name##base##Node *p) {      \
        assert(p);                                                             \
        return (const base##Node *)p;                                          \
    }                                                                          \
    name##base##Node *name##base##Node_cast_node(Node *p) {                    \
        assert(p);                                                             \
        assert(p->kind == NodeKind_##name##base);                              \
        return (name##base##Node *)p;                                          \
    }                                                                          \
    const name##base##Node *name##base##Node_ccast_node(const Node *p) {       \
        assert(p);                                                             \
        assert(p->kind == NodeKind_##name##base);                              \
        return (const name##base##Node *)p;                                    \
    }                                                                          \
    name##base##Node *name##base##Node_cast(base##Node *p) {                   \
        assert(p);                                                             \
        assert(p->kind == NodeKind_##name##base);                              \
        return (name##base##Node *)p;                                          \
    }                                                                          \
    const name##base##Node *name##base##Node_ccast(const base##Node *p) {      \
        assert(p);                                                             \
        assert(p->kind == NodeKind_##name##base);                              \
        return (const name##base##Node *)p;                                    \
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
static void Node_dump_impl(const Node *p, FILE *fp, size_t depth);

static void Node_dump_indent(FILE *fp, size_t depth) {
    assert(fp);

    for (size_t i = 0; i < depth; i++) {
        fprintf(fp, "  ");
    }
}

static void Node_dump_int(long long x, FILE *fp, size_t depth) {
    assert(fp);

    Node_dump_indent(fp, depth);
    fprintf(fp, "(int %lld)\n", x);
}

static void Node_dump_Expr(const ExprNode *x, FILE *fp, size_t depth) {
    assert(fp);

    Node_dump_impl((const Node *)x, fp, depth);
}

static void Node_dump_impl(const Node *p, FILE *fp, size_t depth) {
    assert(fp);

    if (p == NULL) {
        Node_dump_indent(fp, depth);
        fprintf(fp, "(null)\n");
        return;
    }

    switch (p->kind) {
#define NODE(name, base)                                                       \
    case NodeKind_##name##base: {                                              \
        const name##base##Node *q = name##base##Node_ccast_node(p);            \
        (void)q;                                                               \
                                                                               \
        Node_dump_indent(fp, depth);                                           \
        fprintf(fp, "(%s\n", #name #base);

#define NODE_MEMBER_F(T, x, f) Node_dump_##f(q->x, fp, depth + 1);

#define NODE_END()                                                             \
    Node_dump_indent(fp, depth);                                               \
    fprintf(fp, ")\n");                                                        \
    break;                                                                     \
    }
#include "Ast.def"

    default:
        UNREACHABLE();
    }
}

void Node_dump(const Node *p, FILE *fp) {
    assert(fp);

    Node_dump_impl(p, fp, 0);
}
