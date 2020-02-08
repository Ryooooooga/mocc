#include "mocc.h"

void display_usage(const char *program) {
    fprintf(stderr, "%s <INPUT>\n", program);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        display_usage(argv[0]);
        exit(1);
    }

    if (strcmp(argv[1], "--test") == 0) {
        test_Vec();
        test_Path();
        test_File();
        test_Ast();
        test_Lexer();
        test_Preprocessor();
        test_Parser();
        exit(0);
    }

    Vec(String) *include_paths = Vec_new(String)();
    const char *filename = argv[1];
    const char *text = File_read(filename);

    if (text == NULL) {
        ERROR("cannot open file %s\n", filename);
    }

    Vec(Token) *tokens = Preprocessor_read(include_paths, filename, text);
    Parser *p = Parser_new(tokens);

    TranslationUnitNode *node = Parser_parse(p);
    CodeGen_gen(node, stdout);

    return 0;
}
