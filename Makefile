EXE = hot_swap

CC = gcc
WARNINGS =
CFLAGS_RELEASE = -O2

SRC = hot_swap.c cparser.c
OBJS = hot_swap.o cparser.o
OBJS_DIR = .objs

# set up linker
LD = gcc
LDFLAGS = -ldl

release: $(EXE)

$(EXE): hot_swap.o cparser.o
	$(LD) $^ -o $(EXE) $(LDFLAGS)

hot_swap.o: hot_swap.c
	$(CC) $(CFLAGS_RELEASE) -c hot_swap.c

cparser.o: cparser.c
	$(CC) $(CFLAGS_RELEASE) -c cparser.c
