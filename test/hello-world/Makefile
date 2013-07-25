##===- giri/test/hello-world/Makefile --------------------*- Makefile -*-===##
#

CC = clang
CFLAGS = -g -O0 -c -emit-llvm
LDFLAGS = -lm

NAME = hello-world
VERSION ?= Release+Debug+Asserts
GIRI_DIR ?= ../../build/
GIRI_LIB_DIR = $(GIRI_DIR)/$(VERSION)/lib

.PHONY: all lib

all: lib $(NAME).slice

lib:
	$(MAKE) -s -C $(GIRI_DIR)

$(NAME).slice : $(NAME).bc $(NAME).trace
	opt -load $(GIRI_LIB_DIR)/libdgutility.so -load $(GIRI_LIB_DIR)/libgiri.so -bbnum -lsnum -dgiri -tf $(NAME).trace -dfs=true -trace-cd=true -remove-bbnum -remove-lsnum -select-origin-as-main -stats $<  -f -o $@

$(NAME).trace: $(NAME).trace.exe 
	echo 10 | ./$<

$(NAME).trace.exe : $(NAME).trace.s $(GIRI_DIR)/runtime/Giri/$(VERSION)/Tracing.o
	$(CC) -fno-strict-aliasing $+ -o $@ -lpthread $(LDFLAGS)

$(NAME).trace.s : $(NAME).trace.bc 
	llc -asm-verbose=false -O0 $< -o $@

$(NAME).trace.bc : $(NAME).bc
	opt -load $(GIRI_LIB_DIR)/libdgutility.so -load $(GIRI_LIB_DIR)/libgiri.so -mergereturn -bbnum -lsnum -trace-giri -t $(NAME).trace -remove-bbnum -remove-lsnum -debug -stats $< -f -o $@

%.bc : %.c
	$(CC) $(CFLAGS) $+ -o $@

%.ll : %.bc
	llvm-dis $< -o $@

.PHONY: rebuild clean

rebuild: clean all

clean:
	@ rm -f *.ll *.bc *.o *.s *.slice *.exe *.trace
