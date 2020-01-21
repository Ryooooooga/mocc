#include "mocc.h"

#ifdef __APPLE__
#define GLOBAL_PREFIX "_"
#define GLOBAL_POSTFIX "@GOTPCREL"
#else
#define GLOBAL_PREFIX ""
#define GLOBAL_POSTFIX "@GOTPCREL"
#endif

static const char *const registers_qword[6] = {
    "rdi", "rsi", "rdx", "rcx", "r8", "r9"};

static const char *const registers_dword[6] = {
    "edi", "esi", "edx", "ecx", "r8d", "r9d"};

static const char *const registers_byte[6] = {
    "dil", "sil", "dl", "cl", "r8b", "r9b"};

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

typedef struct CodeGen {
    FILE *fp;
    int next_label;
    int return_label;

    Vec(String) * list_of_string;
    Vec(size_t) * list_of_length;
} CodeGen;

static int CodeGen_next_label(CodeGen *g) {
    assert(g);

    return g->next_label++;
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

    for (size_t i = 0; i < Vec_len(String)(g->list_of_string); i++) {
        const char *s = Vec_get(String)(g->list_of_string, i);
        size_t len = Vec_get(size_t)(g->list_of_length, i);

        fprintf(g->fp, ".S%zu:\n", i);

        for (size_t j = 0; j < len; j++) {
            fprintf(g->fp, "  .byte 0x%02x\n", (int)s[j]);
        }
    }
}

static void CodeGen_load_address(CodeGen *g, const NativeAddress *address) {
    assert(address);

    switch (address->type) {
    case NativeAddressType_label:
        fprintf(
            g->fp,
            "  push [rip+%s%s%s]\n",
            GLOBAL_PREFIX,
            address->label,
            GLOBAL_POSTFIX);
        break;

    case NativeAddressType_stack:
        fprintf(g->fp, "  lea rax, [rbp%+d]\n", -address->offset);
        fprintf(g->fp, "  push rax\n");
        break;

    default:
        ERROR("unknown address type %d\n", address->type);
    }
}

static void CodeGen_store(CodeGen *g, const Type *type) {
    assert(g);
    assert(type);

    switch (type->kind) {
    case TypeKind_char:
        fprintf(g->fp, "  pop rdi\n");
        fprintf(g->fp, "  pop rax\n");
        fprintf(g->fp, "  mov [rdi], al\n");
        break;

    case TypeKind_int:
        fprintf(g->fp, "  pop rdi\n");
        fprintf(g->fp, "  pop rax\n");
        fprintf(g->fp, "  mov [rdi], eax\n");
        break;

    case TypeKind_pointer:
        fprintf(g->fp, "  pop rdi\n");
        fprintf(g->fp, "  pop rax\n");
        fprintf(g->fp, "  mov [rdi], rax\n");
        break;

    default:
        ERROR("unknown type\n");
    }
}

static void CodeGen_gen_expr(CodeGen *g, ExprNode *p);
static void CodeGen_gen_stmt(CodeGen *g, StmtNode *p);

// Expressions
static void CodeGen_gen_IdentifierExpr(CodeGen *g, IdentifierExprNode *p) {
    assert(g);
    assert(p);

    CodeGen_load_address(g, p->symbol->address);
}

static void CodeGen_gen_IntegerExpr(CodeGen *g, IntegerExprNode *p) {
    assert(g);
    assert(p);

    fprintf(g->fp, "  push %lld\n", p->value);
}

static void CodeGen_gen_StringExpr(CodeGen *g, StringExprNode *p) {
    assert(g);
    assert(p);

    size_t string_label = CodeGen_add_string(g, p->value, p->length);

    fprintf(g->fp, "  push [rip+.S%zu%s]\n", string_label, GLOBAL_POSTFIX);
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

    if (num_arguments > 6) {
        ERROR("too much arguments\n");
    }

    for (size_t i = 0; i < num_arguments; i++) {
        size_t index = num_arguments - i - 1;
        ExprNode *argument = Vec_get(ExprNode)(p->arguments, index);

        CodeGen_gen_expr(g, argument);
    }

    for (size_t i = 0; i < num_arguments; i++) {
        fprintf(g->fp, "  pop %s\n", registers_qword[i]);
    }

    // Callee
    CodeGen_gen_expr(g, p->callee);

    fprintf(g->fp, "  pop rax\n");
    fprintf(g->fp, "  call rax\n");
    fprintf(g->fp, "  push rax\n");
}

static void CodeGen_gen_UnaryExpr(CodeGen *g, UnaryExprNode *p) {
    assert(g);
    assert(p);

    CodeGen_gen_expr(g, p->operand);

    switch (p->operator) {
    case UnaryOp_address_of:
        break;

    case UnaryOp_indirection:
        break;

    default:
        ERROR("unknown unary op %d", p->operator);
    }
}

static void CodeGen_gen_BinaryExpr(CodeGen *g, BinaryExprNode *p) {
    assert(g);
    assert(p);

    CodeGen_gen_expr(g, p->lhs);
    CodeGen_gen_expr(g, p->rhs);

    fprintf(g->fp, "  pop rdi\n");
    fprintf(g->fp, "  pop rax\n");

    switch (p->operator) {
    case BinaryOp_add:
        fprintf(g->fp, "  add rax, rdi\n");
        fprintf(g->fp, "  push rax\n");
        break;

    case BinaryOp_sub:
        fprintf(g->fp, "  sub rax, rdi\n");
        fprintf(g->fp, "  push rax\n");
        break;

    default:
        ERROR("unknown binary op %d", p->operator);
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

    switch (p->operator) {
    case ImplicitCastOp_lvalue_to_rvalue:
        switch (p->result_type->kind) {
        case TypeKind_char:
            fprintf(g->fp, "  pop rax\n");
            fprintf(g->fp, "  mov al, [rax]\n");
            fprintf(g->fp, "  push rax\n");
            break;

        case TypeKind_int:
            fprintf(g->fp, "  pop rax\n");
            fprintf(g->fp, "  mov eax, [rax]\n");
            fprintf(g->fp, "  push rax\n");
            break;

        case TypeKind_pointer:
            fprintf(g->fp, "  pop rax\n");
            fprintf(g->fp, "  mov rax, [rax]\n");
            fprintf(g->fp, "  push rax\n");
            break;

        default:
            ERROR("unknown type\n");
        }
        break;

    case ImplicitCastOp_function_to_function_pointer:
        break;

    case ImplicitCastOp_array_to_pointer:
        break;

    case ImplicitCastOp_integral_cast:
        switch (p->result_type->kind) {
        case TypeKind_char:
            switch (p->expression->result_type->kind) {
            case TypeKind_int:
                // int -> char
                // Do nothing
                break;

            default:
                UNREACHABLE();
            }
            break;

        case TypeKind_int:
            switch (p->expression->result_type->kind) {
            case TypeKind_char:
                // char -> int
                fprintf(g->fp, "  pop rax\n");
                fprintf(g->fp, "  movsx eax, al\n");
                fprintf(g->fp, "  push rax\n");
                break;

            default:
                UNREACHABLE();
            }
            break;

        default:
            UNREACHABLE();
        }
        break;

    default:
        ERROR("unknown implicit cast operator %d\n", p->operator);
    }
}

static void CodeGen_gen_expr(CodeGen *g, ExprNode *p) {
    assert(g);
    assert(p);

    switch (p->kind) {
#define EXPR_NODE(name)                                                        \
    case NodeKind_##name##Expr:                                                \
        CodeGen_gen_##name##Expr(g, name##ExprNode_cast(p));                   \
        break;
#include "Ast.def"

    default:
        UNREACHABLE();
    }
}

// Statements
static void CodeGen_gen_CompoundStmt(CodeGen *g, CompoundStmtNode *p) {
    assert(g);
    assert(p);

    for (size_t i = 0; i < Vec_len(StmtNode)(p->statements); i++) {
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

    if (p->if_false != NULL) {
        CodeGen_gen_stmt(g, p->if_false);
    }

    // End if
    fprintf(g->fp, ".L%d:\n", end_label);
}

static void CodeGen_gen_ReturnStmt(CodeGen *g, ReturnStmtNode *p) {
    assert(g);
    assert(p);

    if (p->return_value != NULL) {
        CodeGen_gen_expr(g, p->return_value);

        fprintf(g->fp, "  pop rax\n");
    }

    fprintf(g->fp, "  jmp .L%d\n", g->return_label);
}

static void CodeGen_gen_DeclStmt(CodeGen *g, DeclStmtNode *p) {
    assert(g);
    assert(p);

    for (size_t i = 0; i < Vec_len(DeclaratorNode)(p->declarators); i++) {
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

    switch (p->kind) {
#define STMT_NODE(name)                                                        \
    case NodeKind_##name##Stmt:                                                \
        CodeGen_gen_##name##Stmt(g, name##StmtNode_cast(p));                   \
        break;
#include "Ast.def"

    default:
        UNREACHABLE();
    }
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
    *stack_top += size;

    size_t align = Type_alignof(type);
    size_t padding = *stack_top % align == 0 ? 0 : align - *stack_top % align;
    *stack_top += padding;

    return NativeAddress_new_stack(*stack_top);
}

static void CodeGen_allocate_parameters(
    CodeGen *g, Vec(DeclaratorNode) * parameters, int *stack_top) {
    assert(g);
    assert(parameters);
    assert(stack_top);

    for (size_t i = 0; i < Vec_len(DeclaratorNode)(parameters); i++) {
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

    for (size_t i = 0; i < Vec_len(Symbol)(local_variables); i++) {
        Symbol *symbol = Vec_get(Symbol)(local_variables, i);

        symbol->address = CodeGen_alloca_object(g, symbol->type, stack_top);
    }
}

static void
CodeGen_store_parameters(CodeGen *g, Vec(DeclaratorNode) * parameters) {
    assert(g);
    assert(parameters);

    size_t len = Vec_len(DeclaratorNode)(parameters);

    if (len > 6) {
        ERROR("too much parameters\n");
    }

    for (size_t i = 0; i < len; i++) {
        size_t index = len - i - 1;

        DeclaratorNode *parameter = Vec_get(DeclaratorNode)(parameters, index);
        Symbol *symbol = DeclaratorNode_symbol(parameter);

        NativeAddress *address = symbol->address;
        assert(address->type == NativeAddressType_stack);

        switch (symbol->type->kind) {
        case TypeKind_char:
            fprintf(
                g->fp,
                "  mov [rbp%+d], %s\n",
                -address->offset,
                registers_byte[index]);
            break;

        case TypeKind_int:
            fprintf(
                g->fp,
                "  mov [rbp%+d], %s\n",
                -address->offset,
                registers_dword[index]);
            break;

        case TypeKind_pointer:
            fprintf(
                g->fp,
                "  mov [rbp%+d], %s\n",
                -address->offset,
                registers_qword[index]);
            break;

        default:
            ERROR("unknown type");
        }
    }
}

static void CodeGen_gen_global_decl(CodeGen *g, GlobalDeclNode *p) {
    assert(g);
    assert(p);

    for (size_t i = 0; i < Vec_len(DeclaratorNode)(p->declarators); i++) {
        DeclaratorNode *declarator = Vec_get(DeclaratorNode)(p->declarators, i);
        Symbol *symbol = DeclaratorNode_symbol(declarator);
        Type *type = symbol->type;

        if (type->kind == TypeKind_function) {
            if (symbol->address == NULL) {
                symbol->address = NativeAddress_new_label(symbol->name);
            }
            continue;
        }

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

static void CodeGen_gen_function_decl(CodeGen *g, FunctionDeclNode *p) {
    assert(g);
    assert(p);

    // Function label
    Symbol *symbol = DeclaratorNode_symbol(p->declarator);
    symbol->address = NativeAddress_new_label(symbol->name);

    fprintf(g->fp, "  .global %s%s\n", GLOBAL_PREFIX, symbol->name);
    fprintf(g->fp, "%s%s:\n", GLOBAL_PREFIX, symbol->name);

    // Prolog
    fprintf(g->fp, "  push rbp\n");
    fprintf(g->fp, "  mov rbp, rsp\n");

    int stack_top = 0;

    Vec(DeclaratorNode) *parameters = DeclaratorNode_parameters(p->declarator);
    CodeGen_allocate_parameters(g, parameters, &stack_top);
    CodeGen_allocate_local_variables(g, p->local_variables, &stack_top);

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
}

static void CodeGen_gen_top_level_decl(CodeGen *g, DeclNode *p) {
    assert(g);
    assert(p);

    switch (p->kind) {
    case NodeKind_GlobalDecl:
        CodeGen_gen_global_decl(g, GlobalDeclNode_cast(p));
        break;

    case NodeKind_FunctionDecl:
        CodeGen_gen_function_decl(g, FunctionDeclNode_cast(p));
        break;

    default:
        UNREACHABLE();
    }
}

static void CodeGen_gen_translation_unit(CodeGen *g, TranslationUnitNode *p) {
    assert(g);
    assert(p);

    fprintf(g->fp, "  .intel_syntax noprefix\n");
    fprintf(g->fp, "  .text\n");

    for (size_t i = 0; i < Vec_len(DeclNode)(p->declarations); i++) {
        CodeGen_gen_top_level_decl(g, Vec_get(DeclNode)(p->declarations, i));
    }

    CodeGen_gen_constant_pool(g);
}

void CodeGen_gen(TranslationUnitNode *p, FILE *fp) {
    assert(p);
    assert(fp);

    CodeGen g = {
        .fp = fp,
        .next_label = 0,
        .return_label = -1,
        .list_of_string = Vec_new(String)(),
        .list_of_length = Vec_new(size_t)(),
    };

    CodeGen_gen_translation_unit(&g, p);
}
