#!/usr/bin/env bash
BUILD_TYPE=${BUILD_TYPE:-debug}

if [ ! -f "./build/$BUILD_TYPE/mocc" ]; then
    echo "type 'make test BUILD_TYPE=\"$BUILD_TYPE\"' before run test"
    exit 1
fi

dir="$(cd "$(dirname "$0")" || exit 1; pwd)"

try() {
    local test_name=$1
    local input=$2
    local expected=$3

    local asm="$dir/tmp/$test_name.s"
    local bin="$dir/tmp/$test_name"

    local exit_code

    "./build/$BUILD_TYPE/mocc" "$input" > "$asm"
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

    rm "$asm" "$bin"
}

try "c$LINENO" 'int main(void) { return 0; }' 0
try "c$LINENO" 'int main(void) { return 42; }' 42
