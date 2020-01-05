BUILD_TYPE ?= debug

CFLAGS_debug ?= -g -O0
CFLAGS_release ?= -O2 -DNDEBUG
CFLAGS ?= -std=c99 -Wall -Wextra -pedantic -Werror ${CFLAGS_${BUILD_TYPE}}
LDFLAGS ?=

SRCS = \
	main.c \
	Lexer.c \
	test_Lexer.c \
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
