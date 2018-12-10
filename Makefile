EXE = hot_swap

CC = g++
WARNINGS =
CFLAGS_RELEASE = -no-pie -O2

SRC = hot_swap.c cparser.c demo1.c
OBJS = hot_swap.o cparser.o demo1.o
OBJS_DIR = .objs

# set up linker
LD = g++
LDFLAGS = -ldl

.PHONY: release clean

release: demo1

demo1: demo1.cpp hot_swap.cpp cparser.cpp libs/ptrace.c libs/utils.c libs/bin_dlsym.cpp
	$(LD) $(CFLAGS_RELEASE) $^ -o $@ $(LDFLAGS)

demo1.o: demo1.c
	$(CC) $(CFLAGS_RELEASE) -c $<

hot_swap.o: hot_swap.c
	$(CC) $(CFLAGS_RELEASE) -c $<

cparser.o: cparser.c
	$(CC) $(CFLAGS_RELEASE) -c $<

clean:
	rm *.o
