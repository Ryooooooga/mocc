#include "mocc.h"

#ifdef __APPLE__
#define GLOBAL_PREFIX "_"
#define GLOBAL_POSTFIX "@GOTPCREL"
#else
#define GLOBAL_PREFIX ""
#define GLOBAL_POSTFIX "@GOTPCREL"
#endif

enum NativeAddressType {
    NativeAddressType_label,
    NativeAddressType_stack,
};

typedef struct NativeAddress {
    enum NativeAddressType type;

    // For label
    const char *label;

    // For stack
    int offset;
} NativeAddress;

static NativeAddress *NativeAddress_new(enum NativeAddressType type) {
    NativeAddress *a = malloc(sizeof(NativeAddress));
    a->type = type;
    a->offset = 0;

    return a;
}

static NativeAddress *NativeAddress_new_label(const char *label) {
    NativeAddress *a = NativeAddress_new(NativeAddressType_label);
    a->label = label;

    return a;
}

static NativeAddress *NativeAddress_new_stack(int offset) {
    NativeAddress *a = NativeAddress_new(NativeAddressType_stack);
    a->offset = offset;

    return a;
}

#define NUM_REGISTERS 6

typedef struct CodeGen {
    FILE *fp;
    int next_label;
    int return_label;

    Vec(String) * list_of_string;
    Vec(size_t) * list_of_length;

    const char *registers_qword[NUM_REGISTERS];
    const char *registers_dword[NUM_REGISTERS];
    const char *registers_byte[NUM_REGISTERS];
} CodeGen;

static int CodeGen_next_label(CodeGen *g) {
    assert(g);

    int label = g->next_label;
    g->next_label = g->next_label + 1;
    return label;
}

static size_t
CodeGen_add_string(CodeGen *g, const char *string, size_t length) {
    assert(g);
    assert(string);

    // TODO: dedup
    size_t label = Vec_len(String)(g->list_of_string);

    Vec_push(String)(g->list_of_string, string);
    Vec_push(size_t)(g->list_of_length, length);

    return label;
}

static void CodeGen_gen_constant_pool(CodeGen *g) {
    assert(g);

#ifdef __APPLE__
    fprintf(g->fp, "  .section __DATA,__data\n");
#else
    fprintf(g->fp, "  .section .rodata\n");
#endif

    for (size_t i = 0; i < Vec_len(String)(g->list_of_string); i = i + 1) {
        const char *s = Vec_get(String)(g->list_of_string, i);
        size_t len = Vec_get(size_t)(g->list_of_length, i);

        fprintf(g->fp, ".S%zu:\n", i);

        for (size_t j = 0; j < len; j = j + 1) {
            fprintf(g->fp, "  .byte 0x%02x\n", s[j] & 255);
        }
    }
}

static void CodeGen_load_address(CodeGen *g, const NativeAddress *address) {
    assert(address);

    if (address->type == NativeAddressType_label) {
        fprintf(
            g->fp,
            "  mov rax, %s%s%s[rip]\n",
            GLOBAL_PREFIX,
            address->label,
            GLOBAL_POSTFIX);
        fprintf(g->fp, "  push rax\n");
    } else if (address->type == NativeAddressType_stack) {
        fprintf(g->fp, "  lea rax, [rbp%+d]\n", -address->offset);
        fprintf(g->fp, "  push rax\n");
    } else {
        ERROR("unknown address type %d\n", address->type);
    }
}

static void CodeGen_store(CodeGen *g, const Type *type) {
    assert(g);
    assert(type);

    if (type->kind == TypeKind_char) {
        fprintf(g->fp, "  pop rdi\n");
        fprintf(g->fp, "  pop rax\n");
        fprintf(g->fp, "  mov [rdi], al\n");
    } else if (type->kind == TypeKind_int || type->kind == TypeKind_enum) {
        fprintf(g->fp, "  pop rdi\n");
        fprintf(g->fp, "  pop rax\n");
        fprintf(g->fp, "  mov [rdi], eax\n");
    } else if (type->kind == TypeKind_pointer) {
        fprintf(g->fp, "  pop rdi\n");
        fprintf(g->fp, "  pop rax\n");
        fprintf(g->fp, "  mov [rdi], rax\n");
    } else {
        ERROR("unknown type\n");
    }
}

static size_t
CodeGen_member_offset(CodeGen *g, Type *struct_type, Symbol *member_symbol) {
    assert(g);
    assert(struct_type);
    assert(member_symbol);

    (void)g;

    size_t offset = 0;

    for (size_t i = 0; i < Vec_len(Symbol)(struct_type->member_symbols);
         i = i + 1) {
        Symbol *symbol = Vec_get(Symbol)(struct_type->member_symbols, i);
        size_t align = Type_alignof(symbol->type);
        size_t padding = 0;

        if (offset % align != 0) {
            padding = align - offset % align;
        }

        offset = offset + padding;

        if (symbol == member_symbol) {
            return offset;
        }

        offset = offset + Type_sizeof(symbol->type);
    }

    UNREACHABLE();
}

static void CodeGen_gen_expr(CodeGen *g, ExprNode *p);
static void CodeGen_gen_stmt(CodeGen *g, StmtNode *p);

// Expressions
static void CodeGen_gen_IdentifierExpr(CodeGen *g, IdentifierExprNode *p) {
    assert(g);
    assert(p);

    CodeGen_load_address(g, p->symbol->address);
}

static void CodeGen_gen_EnumeratorExpr(CodeGen *g, EnumeratorExprNode *p) {
    assert(g);
    assert(p);

    fprintf(g->fp, "  push %d\n", p->value);
}

static void CodeGen_gen_IntegerExpr(CodeGen *g, IntegerExprNode *p) {
    assert(g);
    assert(p);

    fprintf(g->fp, "  push %d\n", p->value);
}

static void CodeGen_gen_StringExpr(CodeGen *g, StringExprNode *p) {
    assert(g);
    assert(p);

    size_t string_label = CodeGen_add_string(g, p->value, p->length);

    fprintf(g->fp, "  mov rax, .S%zu%s[rip]\n", string_label, GLOBAL_POSTFIX);
    fprintf(g->fp, "  push rax\n");
}

static void CodeGen_gen_SubscriptExpr(CodeGen *g, SubscriptExprNode *p) {
    assert(g);
    assert(p);

    CodeGen_gen_expr(g, p->index);
    CodeGen_gen_expr(g, p->array);

    size_t size = Type_sizeof(p->result_type);

    fprintf(g->fp, "  pop rax\n");
    fprintf(g->fp, "  pop rdi\n");
    fprintf(g->fp, "  imul rdi, %zu\n", size);
    fprintf(g->fp, "  lea rax, [rax+rdi]\n");
    fprintf(g->fp, "  push rax\n");
}

static void CodeGen_gen_CallExpr(CodeGen *g, CallExprNode *p) {
    assert(g);
    assert(p);

    // Arguments
    size_t num_arguments = Vec_len(ExprNode)(p->arguments);

    if (num_arguments > NUM_REGISTERS) {
        ERROR("too much arguments\n");
    }

    for (size_t i = 0; i < num_arguments; i = i + 1) {
        size_t index = num_arguments - i - 1;
        ExprNode *argument = Vec_get(ExprNode)(p->arguments, index);

        CodeGen_gen_expr(g, argument);
    }

    for (size_t i = 0; i < num_arguments; i = i + 1) {
        fprintf(g->fp, "  pop %s\n", g->registers_qword[i]);
    }

    // Callee
    CodeGen_gen_expr(g, p->callee);

    fprintf(g->fp, "  pop r10\n");

    Type *pointer_type = p->callee->result_type;
    Type *function_type = PointerType_pointee_type(pointer_type);

    if (FunctionType_is_var_arg(function_type)) {
        fprintf(g->fp, "  mov rax, 0\n");
    }

    fprintf(g->fp, "  call r10\n");
    fprintf(g->fp, "  push rax\n");
}

static void CodeGen_gen_DotExpr(CodeGen *g, DotExprNode *p) {
    assert(g);
    assert(p);

    if (p->value_category == ValueCategory_rvalue) {
        UNIMPLEMENTED();
    }

    // Parent
    CodeGen_gen_expr(g, p->parent);

    Type *struct_type = p->parent->result_type;

    size_t offset = CodeGen_member_offset(g, struct_type, p->member_symbol);

    fprintf(g->fp, "  pop rax\n");
    fprintf(g->fp, "  add rax, %zu\n", offset);
    fprintf(g->fp, "  push rax\n");
}

static void CodeGen_gen_ArrowExpr(CodeGen *g, ArrowExprNode *p) {
    assert(g);
    assert(p);

    // Parent
    CodeGen_gen_expr(g, p->parent);

    Type *pointer_type = p->parent->result_type;
    Type *struct_type = PointerType_pointee_type(pointer_type);

    size_t offset = CodeGen_member_offset(g, struct_type, p->member_symbol);

    fprintf(g->fp, "  pop rax\n");
    fprintf(g->fp, "  add rax, %zu\n", offset);
    fprintf(g->fp, "  push rax\n");
}

static void CodeGen_gen_SizeofExpr(CodeGen *g, SizeofExprNode *p) {
    assert(g);
    assert(p);

    fprintf(g->fp, "  push %zu\n", Type_sizeof(p->type));
}

static void CodeGen_gen_CastExpr(CodeGen *g, CastExprNode *p) {
    assert(g);
    assert(p);

    CodeGen_gen_expr(g, p->expression);
}

static void CodeGen_gen_UnaryExpr(CodeGen *g, UnaryExprNode *p) {
    assert(g);
    assert(p);

    CodeGen_gen_expr(g, p->operand);

    if (p->operator== UnaryOp_positive) {
        // Do nothing
    } else if (p->operator== UnaryOp_negative) {
        fprintf(g->fp, "  pop rax\n");
        fprintf(g->fp, "  neg rax\n");
        fprintf(g->fp, "  push rax\n");
    } else if (p->operator== UnaryOp_not) {
        fprintf(g->fp, "  pop rax\n");
        fprintf(g->fp, "  cmp rax, 0\n");
        fprintf(g->fp, "  sete al\n");
        fprintf(g->fp, "  movsx rax, al\n");
        fprintf(g->fp, "  push rax\n");
    } else if (p->operator== UnaryOp_address_of) {
        // Do nothing
    } else if (p->operator== UnaryOp_indirection) {
        // Do nothing
    } else {
        ERROR("unknown unary op %d\n", p->operator);
    }
}

static void CodeGen_gen_BinaryExpr(CodeGen *g, BinaryExprNode *p) {
    assert(g);
    assert(p);

    if (p->operator== BinaryOp_add) {
        CodeGen_gen_expr(g, p->lhs);
        CodeGen_gen_expr(g, p->rhs);

        fprintf(g->fp, "  pop rdi\n");
        fprintf(g->fp, "  pop rax\n");
        fprintf(g->fp, "  add rax, rdi\n");
        fprintf(g->fp, "  push rax\n");
    } else if (p->operator== BinaryOp_sub) {
        CodeGen_gen_expr(g, p->lhs);
        CodeGen_gen_expr(g, p->rhs);

        fprintf(g->fp, "  pop rdi\n");
        fprintf(g->fp, "  pop rax\n");
        fprintf(g->fp, "  sub rax, rdi\n");
        fprintf(g->fp, "  push rax\n");
    } else if (p->operator== BinaryOp_mul) {
        CodeGen_gen_expr(g, p->lhs);
        CodeGen_gen_expr(g, p->rhs);

        fprintf(g->fp, "  pop rdi\n");
        fprintf(g->fp, "  pop rax\n");
        fprintf(g->fp, "  imul rax, rdi\n");
        fprintf(g->fp, "  push rax\n");
    } else if (p->operator== BinaryOp_div) {
        CodeGen_gen_expr(g, p->lhs);
        CodeGen_gen_expr(g, p->rhs);

        fprintf(g->fp, "  pop rdi\n");
        fprintf(g->fp, "  pop rax\n");
        fprintf(g->fp, "  cqo\n");
        fprintf(g->fp, "  idiv rdi\n");
        fprintf(g->fp, "  push rax\n");
    } else if (p->operator== BinaryOp_mod) {
        CodeGen_gen_expr(g, p->lhs);
        CodeGen_gen_expr(g, p->rhs);

        fprintf(g->fp, "  pop rdi\n");
        fprintf(g->fp, "  pop rax\n");
        fprintf(g->fp, "  cqo\n");
        fprintf(g->fp, "  idiv rdi\n");
        fprintf(g->fp, "  push rdx\n");
    } else if (p->operator== BinaryOp_lesser_than) {
        CodeGen_gen_expr(g, p->lhs);
        CodeGen_gen_expr(g, p->rhs);

        fprintf(g->fp, "  pop rdi\n");
        fprintf(g->fp, "  pop rax\n");
        fprintf(g->fp, "  cmp rax, rdi\n");
        fprintf(g->fp, "  setl al\n");
        fprintf(g->fp, "  movsx eax, al\n");
        fprintf(g->fp, "  push rax\n");
    } else if (p->operator== BinaryOp_lesser_equal) {
        CodeGen_gen_expr(g, p->lhs);
        CodeGen_gen_expr(g, p->rhs);

        fprintf(g->fp, "  pop rdi\n");
        fprintf(g->fp, "  pop rax\n");
        fprintf(g->fp, "  cmp rax, rdi\n");
        fprintf(g->fp, "  setle al\n");
        fprintf(g->fp, "  movsx eax, al\n");
        fprintf(g->fp, "  push rax\n");
    } else if (p->operator== BinaryOp_greater_than) {
        CodeGen_gen_expr(g, p->lhs);
        CodeGen_gen_expr(g, p->rhs);

        fprintf(g->fp, "  pop rdi\n");
        fprintf(g->fp, "  pop rax\n");
        fprintf(g->fp, "  cmp rax, rdi\n");
        fprintf(g->fp, "  setg al\n");
        fprintf(g->fp, "  movsx eax, al\n");
        fprintf(g->fp, "  push rax\n");
    } else if (p->operator== BinaryOp_greater_equal) {
        CodeGen_gen_expr(g, p->lhs);
        CodeGen_gen_expr(g, p->rhs);

        fprintf(g->fp, "  pop rdi\n");
        fprintf(g->fp, "  pop rax\n");
        fprintf(g->fp, "  cmp rax, rdi\n");
        fprintf(g->fp, "  setge al\n");
        fprintf(g->fp, "  movsx eax, al\n");
        fprintf(g->fp, "  push rax\n");
    } else if (p->operator== BinaryOp_equal) {
        CodeGen_gen_expr(g, p->lhs);
        CodeGen_gen_expr(g, p->rhs);

        fprintf(g->fp, "  pop rdi\n");
        fprintf(g->fp, "  pop rax\n");
        fprintf(g->fp, "  cmp rax, rdi\n");
        fprintf(g->fp, "  sete al\n");
        fprintf(g->fp, "  movsx eax, al\n");
        fprintf(g->fp, "  push rax\n");
    } else if (p->operator== BinaryOp_not_equal) {
        CodeGen_gen_expr(g, p->lhs);
        CodeGen_gen_expr(g, p->rhs);

        fprintf(g->fp, "  pop rdi\n");
        fprintf(g->fp, "  pop rax\n");
        fprintf(g->fp, "  cmp rax, rdi\n");
        fprintf(g->fp, "  setne al\n");
        fprintf(g->fp, "  movsx eax, al\n");
        fprintf(g->fp, "  push rax\n");
    } else if (p->operator== BinaryOp_and) {
        CodeGen_gen_expr(g, p->lhs);
        CodeGen_gen_expr(g, p->rhs);

        fprintf(g->fp, "  pop rdi\n");
        fprintf(g->fp, "  pop rax\n");
        fprintf(g->fp, "  and rax, rdi\n");
        fprintf(g->fp, "  push rax\n");
    } else if (p->operator== BinaryOp_logical_and) {
        int end_label = CodeGen_next_label(g);

        CodeGen_gen_expr(g, p->lhs);

        fprintf(g->fp, "  pop rax\n");
        fprintf(g->fp, "  push 0\n");
        fprintf(g->fp, "  cmp rax, 0\n");
        fprintf(g->fp, "  je .L%d\n", end_label);
        fprintf(g->fp, "  pop rax\n");

        CodeGen_gen_expr(g, p->rhs);

        fprintf(g->fp, "  pop rax\n");
        fprintf(g->fp, "  cmp rax, 0\n");
        fprintf(g->fp, "  setne al\n");
        fprintf(g->fp, "  movsx eax, al\n");
        fprintf(g->fp, "  push rax\n");
        fprintf(g->fp, ".L%d:\n", end_label);
    } else if (p->operator== BinaryOp_logical_or) {
        int end_label = CodeGen_next_label(g);

        CodeGen_gen_expr(g, p->lhs);

        fprintf(g->fp, "  pop rax\n");
        fprintf(g->fp, "  push 1\n");
        fprintf(g->fp, "  cmp rax, 0\n");
        fprintf(g->fp, "  jne .L%d\n", end_label);
        fprintf(g->fp, "  pop rax\n");

        CodeGen_gen_expr(g, p->rhs);

        fprintf(g->fp, "  pop rax\n");
        fprintf(g->fp, "  cmp rax, 0\n");
        fprintf(g->fp, "  setne al\n");
        fprintf(g->fp, "  movsx eax, al\n");
        fprintf(g->fp, "  push rax\n");
        fprintf(g->fp, ".L%d:\n", end_label);
    } else {
        ERROR("unknown binary op %d\n", p->operator);
    }
}

static void CodeGen_gen_AssignExpr(CodeGen *g, AssignExprNode *p) {
    assert(g);
    assert(p);

    CodeGen_gen_expr(g, p->rhs);
    CodeGen_gen_expr(g, p->lhs);

    CodeGen_store(g, p->lhs->result_type);

    fprintf(g->fp, "  push rax\n");
}

static void CodeGen_gen_ImplicitCastExpr(CodeGen *g, ImplicitCastExprNode *p) {
    assert(g);
    assert(p);

    CodeGen_gen_expr(g, p->expression);

    if (p->operator== ImplicitCastOp_lvalue_to_rvalue) {
        if (p->result_type->kind == TypeKind_char) {
            fprintf(g->fp, "  pop rax\n");
            fprintf(g->fp, "  mov al, [rax]\n");
            fprintf(g->fp, "  push rax\n");
        } else if (
            p->result_type->kind == TypeKind_int ||
            p->result_type->kind == TypeKind_enum) {
            fprintf(g->fp, "  pop rax\n");
            fprintf(g->fp, "  mov eax, [rax]\n");
            fprintf(g->fp, "  push rax\n");
        } else if (p->result_type->kind == TypeKind_pointer) {
            fprintf(g->fp, "  pop rax\n");
            fprintf(g->fp, "  mov rax, [rax]\n");
            fprintf(g->fp, "  push rax\n");
        } else {
            ERROR("unknown type\n");
        }
    } else if (p->operator== ImplicitCastOp_function_to_function_pointer) {
        // F -> F*
    } else if (p->operator== ImplicitCastOp_array_to_pointer) {
        // T[] -> T*
    } else if (p->operator== ImplicitCastOp_integral_cast) {
        if (p->result_type->kind == TypeKind_char) {
            if (p->expression->result_type->kind == TypeKind_int) {
                // int -> char
                // Do nothing
            } else {
                UNREACHABLE();
            }
        } else if (
            p->result_type->kind == TypeKind_int ||
            p->result_type->kind == TypeKind_enum) {
            if (p->expression->result_type->kind == TypeKind_char) {
                // char -> int
                fprintf(g->fp, "  pop rax\n");
                fprintf(g->fp, "  movsx eax, al\n");
                fprintf(g->fp, "  push rax\n");
            } else if (
                p->expression->result_type->kind == TypeKind_int ||
                p->expression->result_type->kind == TypeKind_enum) {
                // enum -> int or int -> enum
                // Do nothing
            } else {
                UNREACHABLE();
            }
        } else {
            UNREACHABLE();
        }
    } else if (p->operator== ImplicitCastOp_integer_to_pointer_cast) {
        // int -> T*
    } else if (p->operator== ImplicitCastOp_pointer_to_pointer_cast) {
        // T* -> U*
    } else {
        ERROR("unknown implicit cast operator %d\n", p->operator);
    }
}

static void CodeGen_gen_expr(CodeGen *g, ExprNode *p) {
    assert(g);
    assert(p);

#define EXPR_NODE(name)                                                        \
    if (p->kind == NodeKind_##name##Expr) {                                    \
        CodeGen_gen_##name##Expr(g, name##ExprNode_cast(p));                   \
        return;                                                                \
    }
#include "Ast.def"

    UNREACHABLE();
}

// Statements
static void CodeGen_gen_CompoundStmt(CodeGen *g, CompoundStmtNode *p) {
    assert(g);
    assert(p);

    for (size_t i = 0; i < Vec_len(StmtNode)(p->statements); i = i + 1) {
        CodeGen_gen_stmt(g, Vec_get(StmtNode)(p->statements, i));
    }
}

static void CodeGen_gen_IfStmt(CodeGen *g, IfStmtNode *p) {
    assert(g);
    assert(p);

    int else_label = CodeGen_next_label(g);
    int end_label = CodeGen_next_label(g);

    // Condition
    CodeGen_gen_expr(g, p->condition);

    fprintf(g->fp, "  pop rax\n");
    fprintf(g->fp, "  cmp rax, 0\n");
    fprintf(g->fp, "  je .L%d\n", else_label);

    // Then
    CodeGen_gen_stmt(g, p->if_true);

    fprintf(g->fp, "  jmp .L%d\n", end_label);

    // Else
    fprintf(g->fp, ".L%d:\n", else_label);

    if (p->if_false) {
        CodeGen_gen_stmt(g, p->if_false);
    }

    // End if
    fprintf(g->fp, ".L%d:\n", end_label);
}

static void CodeGen_gen_WhileStmt(CodeGen *g, WhileStmtNode *p) {
    assert(g);
    assert(p);

    int loop_label = CodeGen_next_label(g);
    int condition_label = CodeGen_next_label(g);
    int end_label = CodeGen_next_label(g);

    fprintf(g->fp, "  jmp .L%d\n", condition_label);

    // Body
    fprintf(g->fp, ".L%d:\n", loop_label);

    CodeGen_gen_stmt(g, p->body);

    // Condition
    fprintf(g->fp, ".L%d:\n", condition_label);

    CodeGen_gen_expr(g, p->condition);

    fprintf(g->fp, "  pop rax\n");
    fprintf(g->fp, "  cmp rax, 0\n");
    fprintf(g->fp, "  jne .L%d\n", loop_label);

    fprintf(g->fp, ".L%d:\n", end_label);
}

static void CodeGen_gen_ForStmt(CodeGen *g, ForStmtNode *p) {
    assert(g);
    assert(p);

    int loop_label = CodeGen_next_label(g);
    int condition_label = CodeGen_next_label(g);
    int step_label = CodeGen_next_label(g);
    int end_label = CodeGen_next_label(g);

    // Initializer
    if (p->initializer) {
        CodeGen_gen_stmt(g, p->initializer);
    }

    fprintf(g->fp, "  jmp .L%d\n", condition_label);

    // Body
    fprintf(g->fp, ".L%d:\n", loop_label);

    CodeGen_gen_stmt(g, p->body);

    // Step
    fprintf(g->fp, ".L%d:\n", step_label);

    if (p->step) {
        CodeGen_gen_expr(g, p->step);

        fprintf(g->fp, "  pop rax\n");
    }

    // Condition
    fprintf(g->fp, ".L%d:\n", condition_label);

    if (p->condition) {
        CodeGen_gen_expr(g, p->condition);

        fprintf(g->fp, "  pop rax\n");
        fprintf(g->fp, "  cmp rax, 0\n");
        fprintf(g->fp, "  jne .L%d\n", loop_label);
    } else {
        fprintf(g->fp, "  jmp .L%d\n", loop_label);
    }

    fprintf(g->fp, ".L%d:\n", end_label);
}

static void CodeGen_gen_ReturnStmt(CodeGen *g, ReturnStmtNode *p) {
    assert(g);
    assert(p);

    if (p->return_value) {
        CodeGen_gen_expr(g, p->return_value);

        fprintf(g->fp, "  pop rax\n");
    }

    fprintf(g->fp, "  jmp .L%d\n", g->return_label);
}

static void CodeGen_gen_DeclStmt(CodeGen *g, DeclStmtNode *p) {
    assert(g);
    assert(p);

    for (size_t i = 0; i < Vec_len(DeclaratorNode)(p->declarators); i = i + 1) {
        DeclaratorNode *declarator = Vec_get(DeclaratorNode)(p->declarators, i);

        if (declarator->kind == NodeKind_InitDeclarator) {
            Symbol *symbol = DeclaratorNode_symbol(declarator);
            ExprNode *initializer =
                InitDeclaratorNode_cast(declarator)->initializer;

            CodeGen_gen_expr(g, initializer);
            CodeGen_load_address(g, symbol->address);
            CodeGen_store(g, symbol->type);
        }
    }
}

static void CodeGen_gen_ExprStmt(CodeGen *g, ExprStmtNode *p) {
    assert(g);
    assert(p);

    CodeGen_gen_expr(g, p->expression);

    fprintf(g->fp, "  pop rax\n");
}

static void CodeGen_gen_stmt(CodeGen *g, StmtNode *p) {
    assert(g);
    assert(p);

#define STMT_NODE(name)                                                        \
    if (p->kind == NodeKind_##name##Stmt) {                                    \
        CodeGen_gen_##name##Stmt(g, name##StmtNode_cast(p));                   \
        return;                                                                \
    }
#include "Ast.def"

    UNREACHABLE();
}

// Declarations
static NativeAddress *
CodeGen_alloca_object(CodeGen *g, Type *type, int *stack_top) {
    assert(g);
    assert(stack_top);

    (void)g;

    if (Type_is_incomplete_type(type)) {
        ERROR("cannot allocate an incomplete type\n");
    }

    size_t size = Type_sizeof(type);
    *stack_top = *stack_top + size;

    size_t align = Type_alignof(type);
    size_t padding = 0;

    if (*stack_top % align != 0) {
        padding = align - *stack_top % align;
    }

    *stack_top = *stack_top + padding;

    return NativeAddress_new_stack(*stack_top);
}

static void CodeGen_allocate_parameters(
    CodeGen *g, Vec(DeclaratorNode) * parameters, int *stack_top) {
    assert(g);
    assert(parameters);
    assert(stack_top);

    for (size_t i = 0; i < Vec_len(DeclaratorNode)(parameters); i = i + 1) {
        DeclaratorNode *parameter = Vec_get(DeclaratorNode)(parameters, i);
        Symbol *symbol = DeclaratorNode_symbol(parameter);

        symbol->address = CodeGen_alloca_object(g, symbol->type, stack_top);
    }
}

static void CodeGen_allocate_local_variables(
    CodeGen *g, Vec(Symbol) * local_variables, int *stack_top) {
    assert(g);
    assert(local_variables);
    assert(stack_top);

    for (size_t i = 0; i < Vec_len(Symbol)(local_variables); i = i + 1) {
        Symbol *symbol = Vec_get(Symbol)(local_variables, i);

        symbol->address = CodeGen_alloca_object(g, symbol->type, stack_top);
    }
}

static void
CodeGen_store_parameters(CodeGen *g, Vec(DeclaratorNode) * parameters) {
    assert(g);
    assert(parameters);

    size_t len = Vec_len(DeclaratorNode)(parameters);

    if (len > NUM_REGISTERS) {
        ERROR("too much parameters\n");
    }

    for (size_t i = 0; i < len; i = i + 1) {
        size_t index = len - i - 1;

        DeclaratorNode *parameter = Vec_get(DeclaratorNode)(parameters, index);
        Symbol *symbol = DeclaratorNode_symbol(parameter);

        NativeAddress *address = symbol->address;
        assert(address->type == NativeAddressType_stack);

        if (symbol->type->kind == TypeKind_char) {
            fprintf(
                g->fp,
                "  mov [rbp%+d], %s\n",
                -address->offset,
                g->registers_byte[index]);
        } else if (
            symbol->type->kind == TypeKind_int ||
            symbol->type->kind == TypeKind_enum) {
            fprintf(
                g->fp,
                "  mov [rbp%+d], %s\n",
                -address->offset,
                g->registers_dword[index]);
        } else if (symbol->type->kind == TypeKind_pointer) {
            fprintf(
                g->fp,
                "  mov [rbp%+d], %s\n",
                -address->offset,
                g->registers_qword[index]);
        } else {
            ERROR("unknown type\n");
        }
    }
}

static void CodeGen_gen_global_decl(CodeGen *g, GlobalDeclNode *p) {
    assert(g);
    assert(p);

    for (size_t i = 0; i < Vec_len(DeclaratorNode)(p->declarators); i = i + 1) {
        DeclaratorNode *declarator = Vec_get(DeclaratorNode)(p->declarators, i);
        Symbol *symbol = DeclaratorNode_symbol(declarator);

        if (symbol->storage_class != StorageClass_typedef) {
            assert(symbol->name);

            Type *type = symbol->type;

            if (type->kind == TypeKind_function) {
                if (!symbol->address) {
                    symbol->address = NativeAddress_new_label(symbol->name);
                }
            } else {
                assert(!Type_is_incomplete_type(type));

                symbol->address = NativeAddress_new_label(symbol->name);

                // TODO: initializer
                fprintf(
                    g->fp,
                    "  .comm %s%s, %zu, %zu\n",
                    GLOBAL_PREFIX,
                    symbol->name,
                    Type_sizeof(type),
                    Type_alignof(type));
            }
        }
    }
}

static void CodeGen_gen_function_decl(CodeGen *g, FunctionDeclNode *p) {
    assert(g);
    assert(p);

    // Function label
    Symbol *symbol = DeclaratorNode_symbol(p->declarator);
    symbol->address = NativeAddress_new_label(symbol->name);

    if (symbol->storage_class != StorageClass_static) {
        fprintf(g->fp, "  .global %s%s\n", GLOBAL_PREFIX, symbol->name);
    }
    fprintf(g->fp, "%s%s:\n", GLOBAL_PREFIX, symbol->name);
    fprintf(g->fp, "  .cfi_startproc\n");

    // Prolog
    fprintf(g->fp, "  push rbp\n");
    fprintf(g->fp, "  .cfi_def_cfa_offset 16\n");
    fprintf(g->fp, "  .cfi_offset rbp, -16\n");
    fprintf(g->fp, "  mov rbp, rsp\n");
    fprintf(g->fp, "  .cfi_def_cfa_register rbp\n");

    int stack_top = 0;

    Vec(DeclaratorNode) *parameters = DeclaratorNode_parameters(p->declarator);
    CodeGen_allocate_parameters(g, parameters, &stack_top);
    CodeGen_allocate_local_variables(g, p->local_variables, &stack_top);

    if (stack_top % 16 != 0) {
        stack_top = (stack_top / 16 + 1) * 16;
    }

    fprintf(g->fp, "  sub rsp, %d\n", stack_top);

    CodeGen_store_parameters(g, parameters);

    // Body
    g->return_label = CodeGen_next_label(g);

    CodeGen_gen_stmt(g, p->body);

    // Epilog
    fprintf(g->fp, ".L%d:\n", g->return_label);
    fprintf(g->fp, "  mov rsp, rbp\n");
    fprintf(g->fp, "  pop rbp\n");
    fprintf(g->fp, "  ret\n");
    fprintf(g->fp, "  .cfi_endproc\n");
}

static void CodeGen_gen_top_level_decl(CodeGen *g, DeclNode *p) {
    assert(g);
    assert(p);

    if (p->kind == NodeKind_GlobalDecl) {
        CodeGen_gen_global_decl(g, GlobalDeclNode_cast(p));
    } else if (p->kind == NodeKind_FunctionDecl) {
        CodeGen_gen_function_decl(g, FunctionDeclNode_cast(p));
    } else {
        UNREACHABLE();
    }
}

static void CodeGen_gen_translation_unit(CodeGen *g, TranslationUnitNode *p) {
    assert(g);
    assert(p);

    fprintf(g->fp, "  .intel_syntax noprefix\n");
    fprintf(g->fp, "  .text\n");

    for (size_t i = 0; i < Vec_len(DeclNode)(p->declarations); i = i + 1) {
        CodeGen_gen_top_level_decl(g, Vec_get(DeclNode)(p->declarations, i));
    }

    CodeGen_gen_constant_pool(g);
}

void CodeGen_gen(TranslationUnitNode *p, FILE *fp) {
    assert(p);
    assert(fp);

    CodeGen g;
    g.fp = fp;
    g.next_label = 0;
    g.return_label = -1;
    g.list_of_string = Vec_new(String)();
    g.list_of_length = Vec_new(size_t)();

    g.registers_qword[0] = "rdi";
    g.registers_qword[1] = "rsi";
    g.registers_qword[2] = "rdx";
    g.registers_qword[3] = "rcx";
    g.registers_qword[4] = "r8";
    g.registers_qword[5] = "r9";

    g.registers_dword[0] = "edi";
    g.registers_dword[1] = "esi";
    g.registers_dword[2] = "edx";
    g.registers_dword[3] = "ecx";
    g.registers_dword[4] = "r8d";
    g.registers_dword[5] = "r9d";

    g.registers_byte[0] = "dil";
    g.registers_byte[1] = "sil";
    g.registers_byte[2] = "dl";
    g.registers_byte[3] = "cl";
    g.registers_byte[4] = "r8b";
    g.registers_byte[5] = "r9b";

    CodeGen_gen_translation_unit(&g, p);
}
