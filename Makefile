CC ?= gcc
LIBS=-lpng -lm
OPTS=-Ofast -march=native

all : idealplace

% : %.c
	${CC} ${OPTS} -o $@ $< ${LIBS}
