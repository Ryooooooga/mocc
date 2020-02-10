#include "mocc.h"

void display_usage(const char *program) {
    fprintf(stderr, "%s <INPUT> <OUTPUT>\n", program);
}

int main(int argc, char *argv[]) {
    if (argc == 2 && strcmp(argv[1], "--test") == 0) {
        test_Vec();
        test_Path();
        test_File();
        test_Ast();
        test_Lexer();
        test_Preprocessor();
        test_Parser();
        exit(0);
    }

    if (argc != 3) {
        display_usage(argv[0]);
        exit(1);
    }

    Vec(String) *include_paths = Vec_new(String)();
    const char *input = argv[1];
    const char *output = argv[2];

    const char *text = File_read(input);
    if (text == (const char *)NULL) {
        ERROR("cannot open file %s\n", input);
    }

    FILE *fp = fopen(output, "w");
    if (fp == (FILE *)NULL) {
        ERROR("cannot open file %s\n", output);
    }

    Vec(Token) *tokens = Preprocessor_read(include_paths, input, text);
    Parser *p = Parser_new(tokens);

    TranslationUnitNode *node = Parser_parse(p);
    CodeGen_gen(node, fp);

    fclose(fp);

    return 0;
}
