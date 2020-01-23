int printf(const char *format, ...);
int atoi(const char *s);

int main(int argc, char **argv) {
    if (argc == 1) {
        printf("%s <N>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);

    for (int i = 1; i <= n; i = i + 1) {
        if (i % 15 == 0) {
            printf("Fizz Buzz\n");
        } else if (i % 3 == 0) {
            printf("Fizz\n");
        } else if (i % 5 == 0) {
            printf("Buzz\n");
        } else {
            printf("%d\n", i);
        }
    }

    return 0;
}
