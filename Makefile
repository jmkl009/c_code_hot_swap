EXE = hot_swap

CC = g++
WARNINGS =
CFLAGS_RELEASE = -no-pie -O2
CFLAGS_DEBUG = -no-pie -g

SRC = hot_swap.cpp cparser.cpp demo1.cpp
OBJS = hot_swap.o cparser.o demo1.o
OBJS_DIR = .objs

# set up linker
LD = g++
LDFLAGS_DEMO = -ldl -lunwind -lunwind-ptrace -lunwind-generic
LDFLAGS = -ldl -lunwind -lunwind-ptrace -lunwind-generic

.PHONY: release clean

release: demo1 ddb

demo1: demo1.cpp hot_swap.cpp cparser.cpp libs/ptrace.c libs/utils.c libs/bin_dlsym.cpp FunctionInjector.cpp
	$(LD) $(CFLAGS_RELEASE) $^ -o $@ $(LDFLAGS_DEMO)

ddb: ddb.cpp hot_swap.cpp cparser.cpp libs/ptrace.c libs/utils.c libs/bin_dlsym.cpp FunctionInjector.cpp StackUnwinder.cpp
	$(LD) $(CFLAGS_RELEASE) $^ -o $@ $(LDFLAGS)

hot_swap.o:
	$(CC) $(CFLAGS_RELEASE) -c $<


clean:
	rm *.o
