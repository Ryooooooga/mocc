#include <stdio.h>
#include <stdlib.h>

void display_usage(const char *program) {
    fprintf(stderr, "%s <INPUT>\n", program);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        display_usage(argv[0]);
        exit(1);
    }

    printf("Hello, world!\n");

    return 0;
}
