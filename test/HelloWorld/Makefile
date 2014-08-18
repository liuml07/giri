##===- giri/test/UnitTests/Makefile.common -----------------*- Makefile -*-===##
#

DEBUGFLAGS ?= -debug
CFLAGS ?= -g -O0 -c -emit-llvm
LDFLAGS ?=
BuildMode ?= Release+Debug+Asserts
GIRI_DIR ?= ../../build/
NAME ?= hello-world

CC = clang
CXX = clang++
GIRI_LIB_DIR = $(GIRI_DIR)/$(BuildMode)/lib
GIRI_BIN_DIR = $(GIRI_DIR)/$(BuildMode)/bin

.PHONY: all lib

all: lib $(NAME).slice
	@ echo -n "The line numbers of source code slice are: "
	@ sed -n '/^Source.*[0-9]$$/p' $(NAME).slice |\
		awk -F: '{print $$3}' |\
		sort -g |\
		uniq | xargs\
		echo

lib:
	$(MAKE) -s -C $(GIRI_DIR)

$(NAME).slice : $(NAME).bc $(NAME).trace
	@ opt -load $(GIRI_LIB_DIR)/libdgutility.so \
		-load $(GIRI_LIB_DIR)/libgiri.so \
		-mergereturn -bbnum -lsnum \
		-dgiri -trace-file=$(NAME).trace -slice-file=$(NAME).slice \
		-remove-bbnum -remove-lsnum \
		-stats $(DEBUGFLAGS) $< -o /dev/null

$(NAME).trace: $(NAME).trace.exe
	- ./$< $(INPUT)

$(NAME).trace.exe : $(NAME).trace.s
	$(CXX) -fno-strict-aliasing $+ -o $@ $(LDFLAGS) -L$(GIRI_LIB_DIR) -lrtgiri

$(NAME).trace.s : $(NAME).trace.bc
	llc -asm-verbose=false -O0 $< -o $@

$(NAME).trace.bc : $(NAME).bc
	@ opt -load $(GIRI_LIB_DIR)/libdgutility.so \
		-load $(GIRI_LIB_DIR)/libgiri.so \
		-mergereturn -bbnum -lsnum \
		-trace-giri -trace-file=$(NAME).trace \
		-remove-bbnum -remove-lsnum \
		-stats $(DEBUGFLAGS) $< -o $@

%.bc : %.c
	$(CC) $(CFLAGS) $+ -o $@
%.ll : %.bc
	llvm-dis $< -o $@

.PHONY: ptrace rebuild clean

prtrace: $(NAME).trace
	$(GIRI_BIN_DIR)/prtrace $<

rebuild: clean all

clean:
	@ rm -f *.ll *.bc *.o *.s *.slice *.exe *.trace
