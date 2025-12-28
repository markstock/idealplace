CC ?= gcc
OPTS=-Ofast -march=native

# use pkg-config to find libpng
PNG_CFLAGS := $(shell pkg-config --cflags libpng 2>/dev/null)
PNG_LIBS   := $(shell pkg-config --libs libpng 2>/dev/null || echo "-lpng")

CFLAGS+=$(OPTS) $(PNG_CFLAGS)
LIBS=$(PNG_LIBS) -lm

all : idealplace

% : %.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

clean :
	rm -f idealplace
