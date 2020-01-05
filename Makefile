BUILD_TYPE ?= debug

CFLAGS_debug ?= -g -O0
CFLAGS_release ?= -O2 -DNDEBUG
CFLAGS ?= -std=c99 -Wall -Wextra -pedantic -Werror ${CFLAGS_${BUILD_TYPE}}
LDFLAGS ?=

SRCS = \
	main.c \
	Vec.c \
	Lexer.c \
	Preprocessor.c \
	test_Vec.c \
	test_Lexer.c \
	test_Preprocessor.c \
	# -- SRCS

OBJS = ${SRCS:%=%.o}

all: mocc

test: mocc
	./mocc --test

clean:
	${RM} *.o *.d mocc

mocc: ${OBJS}
	${CC} ${CFLAGS} -o $@ $^ ${LDFLAGS}

%.c.o: %.c
	${CC} ${CFLAGS} -MMD -MP -o $@ -c $<

.PHONY: all test clean

-include ${OBJS:.o=.d}
