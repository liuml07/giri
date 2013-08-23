##===- giri/test/matrix_multiply/Makefile -----------------*- Makefile -*-===##

ifeq (pthread,$(VERSION))
NAME = matrix_multiply-pthread
LDFLAGS = -lpthread
else
NAME = matrix_multiply-seq
endif

GIRI_DIR = ../../build/
INPUT ?= 16

include ../UnitTests/Makefile.common

clean-all:
	@ rm -f matrix_file_out_*.txt
