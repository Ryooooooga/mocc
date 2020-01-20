#!/usr/bin/env bash
BUILD_TYPE=${BUILD_TYPE:-debug}

if [ ! -f "./build/$BUILD_TYPE/mocc" ]; then
    echo "type 'make BUILD_TYPE=\"$BUILD_TYPE\"' before run test"
    exit 1
fi

dir="$(cd "$(dirname "$0")" || exit 1; pwd)"

try() {
    local test_name=$1
    local input=$2
    local expected=$3

    local c="$dir/tmp/$test_name.c"
    local asm="$dir/tmp/$test_name.s"
    local bin="$dir/tmp/$test_name"

    local exit_code

    echo -n "$input" > "$c"

    "./build/$BUILD_TYPE/mocc" "$(cat "$c")" > "$asm"
    exit_code="$?"
    if [ "$exit_code" -ne 0 ]; then
        echo "$test_name: compilation failed with exit code $exit_code"
        exit 1
    fi

    gcc "$asm" -o "$bin"
    exit_code="$?"
    if [ "$exit_code" -ne 0 ]; then
        echo "$test_name: assemble failed"
        exit 1
    fi

    "$bin"
    exit_code="$?"
    if [ "$exit_code" -ne "$expected" ]; then
        echo "$test_name: expected $expected, actual $exit_code"
        exit 1
    fi

    rm "$c" "$asm" "$bin"
}

try "c$LINENO" '
    int main(void) {
        return 0;
    }' 0

try "c$LINENO" '
    int main(void) {
        return 42;
    }' 42

try "c$LINENO" '
    int main(void) {
        int a;
        a = 2;
        return a;
    }' 2

try "c$LINENO" '
    int main(void) {
        int a = 3;
        int b = a;
        return b;
    }' 3

try "c$LINENO" '
    int main(void) {
        int a = 5 + 2;
        return a + a - 6 - 3;
    }' 5

try "c$LINENO" '
    int main(void) {
        int a;
        *&a = 5;
        return *&a;
    }' 5

try "c$LINENO" '
    int main(void) {
        int a = 5;
        int *p = &a;
        return *p;
    }' 5

try "c$LINENO" '
    int main(void) {
        int a;
        int *p = &a;
        *p = 10;
        return a;
    }' 10

try "c$LINENO" '
    int f(void) { return 42; }
    int main(void) { return f(); }
    ' 42

try "c$LINENO" '
    int f(void) { return 10; }
    int g(void) { return f() + 2; }
    int main(void) { return g(); }
    ' 12

try "c$LINENO" '
    int f(int x) { return x + x; }
    int main(void) { return f(4); }
    ' 8

try "c$LINENO" '
    int f(int x, int y) { return x + y; }
    int g(int x, int y, int z) { return x + y + z; }
    int main(void) { return f(4, 6) - g(2, 3, 5); }
    ' 0

try "c$LINENO" '
    int f(int x) {
        if (x) return x;
        else return 10;
    }
    int main(void) { return f(0) + f(8); }
    ' 18

try "c$LINENO" '
    int f(int x) {
        if (x - 0)
            if (x - 1) return f(x - 1) + f(x - 2);
            else return 1;
        return 0;
    }
    int main(void) { return f(9); }
    ' 34

try "c$LINENO" '
    int main(void) {
        int a = 5, *p = &a;
        return p[0];
    }' 5

try "c$LINENO" '
    int main(void) {
        int a, *p = &a;
        p[0] = 4;
        return a;
    }' 4

try "c$LINENO" '
    int main(void) {
        int a[1];
        a[0] = 4;
        return a[0];
    }' 4

try "c$LINENO" '
    int main(void) {
        int a[3];
        a[0] = 4;
        a[1] = 7;
        a[2] = 11;
        return a[2] + a[0];
    }' 15

try "c$LINENO" '
    int main(void) {
        int a[2][3];
        a[0][0] = 3; a[0][1] = 4; a[0][2] = 5;
        a[1][0] = 6; a[1][1] = 7; a[1][2] = 8;
        return *a[0] + *a[1];
    }' 9

try "c$LINENO" '
    int main(void) {
        char a = 257;
        return a;
    }' 1

try "c$LINENO" '
    char f(char c) { return c; }
    int main(void) {
        char a = f(258);
        return a;
    }' 2

try "c$LINENO" '
    int f(char c) { return c + 1; }
    int main(void) {
        char a = f(258);
        return a;
    }' 3

try "c$LINENO" '
    int;
    int main(void) { return 0; }
    ' 0
