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
        test_Lexer();
        exit(0);
    }

    const char *filename = "<input>";
    const char *text = argv[1]; // TODO: Read from file

    Lexer *l = Lexer_new(filename, text);

    Token *t;
    do {
        t = Lexer_read(l);

        fprintf(stderr, "token(%d, %s)\n", t->kind, t->text);
    } while (t->kind != '\0');

    return 0;
}
