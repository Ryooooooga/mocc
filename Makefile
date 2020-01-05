BUILD_TYPE ?= debug

CFLAGS_debug ?= -g -O0
CFLAGS_release ?= -O2 -DNDEBUG
CFLAGS ?= -std=c99 -Wall -Wextra -pedantic -Werror ${CFLAGS_${BUILD_TYPE}}
LDFLAGS ?=

SRCS = main.c
OBJS = ${SRCS:.c=.o}

all: mocc

test: mocc
	./mocc

clean:
	${RM} *.o *.d mocc

mocc: ${OBJS}
	${CC} ${CFLAGS} -o $@ $^ ${LDFLAGS}

.PHONY: all test clean
