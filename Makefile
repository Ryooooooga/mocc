BUILD_TYPE ?= debug

CFLAGS_debug ?= -g -O0
CFLAGS_release ?= -O2 -DNDEBUG
CFLAGS ?= -std=c99 -Wall -Wextra -pedantic -Werror ${CFLAGS_${BUILD_TYPE}}
LDFLAGS ?=

SRCS = \
	main.c \
	Vec.c \
	File.c \
	Ast.c \
	Lexer.c \
	Preprocessor.c \
	Parser.c \
	test_Vec.c \
	test_File.c \
	test_Ast.c \
	test_Lexer.c \
	test_Preprocessor.c \
	test_Parser.c \
	# -- SRCS

BUILD_DIR = build
OBJS = ${SRCS:%=${BUILD_DIR}/${BUILD_TYPE}/%.o}
TARGET = ${BUILD_DIR}/${BUILD_TYPE}/mocc

all: ${TARGET}

test: ${TARGET}
	./${TARGET} --test

clean:
	${RM} ${BUILD_DIR}

${TARGET}: ${OBJS}
	@mkdir -p ${@D}
	${CC} ${CFLAGS} -o $@ $^ ${LDFLAGS}

${BUILD_DIR}/${BUILD_TYPE}/%.c.o: %.c
	@mkdir -p ${@D}
	${CC} ${CFLAGS} -MMD -MP -o $@ -c $<

.PHONY: all test clean

-include ${OBJS:.o=.d}
