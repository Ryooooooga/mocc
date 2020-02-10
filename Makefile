BUILD_TYPE ?= debug

CFLAGS_debug ?= -g -O0
CFLAGS_release ?= -O2 -DNDEBUG
CFLAGS ?= -std=c11 -Wall -Wextra -pedantic -Werror ${CFLAGS_${BUILD_TYPE}}
LDFLAGS ?=

SRCS = \
	main.c \
	Vec.c \
	Path.c \
	File.c \
	Type.c \
	Symbol.c \
	Scope.c \
	Ast.c \
	Lexer.c \
	Preprocessor.c \
	Parser.c \
	Sema.c \
	CodeGen.c \
	test_Vec.c \
	test_Path.c \
	test_File.c \
	test_Ast.c \
	test_Lexer.c \
	test_Preprocessor.c \
	test_Parser.c \
	# -- SRCS

SRC_DIR = src
BUILD_DIR = build
OBJS = ${SRCS:%=${BUILD_DIR}/${BUILD_TYPE}/%.o}
TARGET = ${BUILD_DIR}/${BUILD_TYPE}/mocc

SRCS2 = \
	main.c \
	Vec.c \
	Path.c \
	File.c \
	Type.c \
	${BUILD_DIR}/${SRC_DIR}/Symbol.s \
	Scope.c \
	Ast.c \
	Lexer.c \
	Preprocessor.c \
	Parser.c \
	Sema.c \
	CodeGen.c \
	test_Vec.c \
	test_Path.c \
	test_File.c \
	test_Ast.c \
	test_Lexer.c \
	test_Preprocessor.c \
	test_Parser.c \
	# -- SRCS2

OBJS2 = ${SRCS2:%=${BUILD_DIR}/${BUILD_TYPE}/%.o}

all: ${TARGET} ${TARGET}_stage2

test: ${TARGET} ${TARGET}_stage2
	./${TARGET} --test
	MOCC=${TARGET} ./test.bash
	./${TARGET}_stage2 --test
	MOCC=${TARGET}_stage2 ./test.bash

clean:
	${RM} -r ${BUILD_DIR} tmp/*

${TARGET}: ${OBJS}
	@echo "linking $@"
	@mkdir -p ${@D}
	@${CC} ${CFLAGS} -o $@ $^ ${LDFLAGS}

 ${TARGET}_stage2: ${OBJS2}
	@echo "linking $@"
	@mkdir -p ${@D}
	@${CC} ${CFLAGS} -o $@ $^ ${LDFLAGS}

${BUILD_DIR}/${BUILD_TYPE}/%.c.o: %.c
	@echo "compiling $<"
	@mkdir -p ${@D}
	@${CC} ${CFLAGS} -MMD -MP -o $@ -c $<

${BUILD_DIR}/${BUILD_TYPE}/%.s.o: %.s
	@echo "assembling $<"
	@mkdir -p ${@D}
	@${CC} ${CFLAGS} -o $@ -c $<

${BUILD_DIR}/${SRC_DIR}/%.c: %.c mocc.h
	@echo "precompiling $<"
	@mkdir -p ${@D}
	@${CPP} -P -DMOCC -o $@ $<

${BUILD_DIR}/${SRC_DIR}/%.s: ${BUILD_DIR}/${SRC_DIR}/%.c ${TARGET}
	@echo "compiling $<"
	@mkdir -p ${@D}
	@${TARGET} $< > $@

.PHONY: all test clean

-include ${OBJS:.o=.d}
