#include "mocc.h"

void test_File(void) {
    {
        char *s = File_read("test/hello.c");
        (void)s;

        assert(s != NULL);
        assert(strcmp(
            s,
            "int puts(const char *p);\n"
            "int main(void) {\n"
            "    puts(\"Hello, wolrd!\");\n"
            "    return 0;\n"
            "}\n"));
    }
    {
        char *s = File_read("NO_SUCH_FILE");
        (void)s;

        assert(s == NULL);
    }
}
