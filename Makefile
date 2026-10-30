.RECIPEPREFIX = >
CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -Wno-deprecated-declarations
LIB = src/board.c src/nnue.c src/search.c

all: cartman perft

cartman: $(LIB) src/uci.c
> $(CC) $(CFLAGS) -Isrc -o cartman $(LIB) src/uci.c

perft: src/board.c tests/perft.c
> $(CC) $(CFLAGS) -Isrc -o perft src/board.c tests/perft.c

clean:
> rm -f perft cartman
