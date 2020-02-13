BUILD_TYPE ?= debug

CFLAGS_debug ?= -g -O0
CFLAGS_release ?= -O2 -DNDEBUG
CFLAGS ?= -std=c11 -Wall -Wextra -pedantic -Werror ${CFLAGS_${BUILD_TYPE}}
LDFLAGS ?=

BUILD_DIR = build

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
	# -- SRCS

TEST_SRCS = \
	test_Vec.c \
	test_Path.c \
	test_File.c \
	test_Ast.c \
	test_Lexer.c \
	test_Preprocessor.c \
	test_Parser.c \
	# -- TEST_SRCS

STAGE1_SRCS = ${SRCS} ${TEST_SRCS}
STAGE1_OBJS = ${STAGE1_SRCS:%=${BUILD_DIR}/${BUILD_TYPE}/stage1/%.o}
STAGE1_TARGET = ${BUILD_DIR}/${BUILD_TYPE}/stage1/mocc
STAGE2_TARGET = ${BUILD_DIR}/${BUILD_TYPE}/stage2/mocc
STAGE3_TARGET = ${BUILD_DIR}/${BUILD_TYPE}/stage3/mocc

all: ${STAGE3_TARGET}

test: ${STAGE3_TARGET}
	./${STAGE1_TARGET} --test
	MOCC=${STAGE1_TARGET} ./test.bash
	MOCC=${STAGE2_TARGET} ./test.bash
	MOCC=${STAGE3_TARGET} ./test.bash
	cmp ${STAGE2_TARGET} ${STAGE3_TARGET}

clean:
	${RM} -r ${BUILD_DIR} tmp/*

${STAGE1_TARGET}: ${STAGE1_OBJS}
	@echo "linking $@"
	@mkdir -p ${@D}
	@${CC} ${CFLAGS} -o $@ $^ ${LDFLAGS}

${BUILD_DIR}/${BUILD_TYPE}/stage1/%.c.o: %.c
	@echo "compiling $<"
	@mkdir -p ${@D}
	@${CC} ${CFLAGS} -MMD -MP -o $@ -c $<

${BUILD_DIR}/${BUILD_TYPE}/stage%/Makefile: Makefile.stage
	@echo "generating $@"
	@mkdir -p ${@D}
	@cp $< $@

.PHONY: all test clean

${STAGE2_TARGET}: ${STAGE1_TARGET} ${BUILD_DIR}/${BUILD_TYPE}/stage2/Makefile
	${MAKE} -C ${BUILD_DIR}/${BUILD_TYPE}/stage2 MOCC=${CURDIR}/${STAGE1_TARGET}

${STAGE3_TARGET}: ${STAGE2_TARGET} ${BUILD_DIR}/${BUILD_TYPE}/stage3/Makefile
	${MAKE} -C ${BUILD_DIR}/${BUILD_TYPE}/stage3 MOCC=${CURDIR}/${STAGE2_TARGET}

-include ${OBJS:.o=.d}
