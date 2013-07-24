##===- giri/test/hello-world/Makefile --------------------*- Makefile -*-===##
#

CC = clang
CFLAGS = -g -O0 -c -emit-llvm
LD_FLAGS = -lm

NAME = hello-world
VERSION = Release+Asserts
GIRI_DIR = ../../build/
GIRI_LIB_DIR = $(GIRI_DIR)/$(VERSION)/lib

all: lib $(NAME).slice.txt

lib:
	$(MAKE) -s -C $(GIRI_DIR)

$(NAME).trace.out : $(NAME).trace.s $(GIRI_DIR)/runtime/giri/$(VERSION)/Tracing.o
	$(CC) -fno-strict-aliasing $+ -o $@ -lpthread

$(NAME).trace.s : $(NAME).trace.bc 
	llc -asm-verbose=false -O0 $< -o $@

$(NAME).trace.bc : $(NAME).bc
	opt -load $(GIRI_LIB_DIR)/libdgutility.so -load $(GIRI_LIB_DIR)/libgiri.so -mergereturn -bbnum -lsnum -trace-giri -t bbrecord -inv-insert -remove-bbnum -remove-lsnum -debug -stats $< -f -o $@

$(NAME).slice.txt : $(NAME).bc
	opt -load $(GIRI_LIB_DIR)/libdgutility.so -load $(GIRI_LIB_DIR)/libgiri.so -bbnum -lsnum -dgiri -tf bbrecord -dfs=true -inv-filter -trace-cd=true -remove-bbnum -remove-lsnum -select-origin-as-main -stats $<  -f -o  /dev/null  >& $@

%.bc : %.c
	$(CC) $(CFLAGS) $+ -o $@

%.ll : %.bc
	llvm-dis $< -o $@

clean:
	@ rm -f *.ll *.bc *.o *.s *.txt
