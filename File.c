#include "mocc.h"

char *File_read(const char *path) {
    assert(path);

    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (size < 0) {
        fclose(fp);
        return NULL;
    }

    char *s = malloc(sizeof(char) * (size + 1));
    fread(s, 1, size, fp);
    s[size] = '\0';

    fclose(fp);
    return s;
}
