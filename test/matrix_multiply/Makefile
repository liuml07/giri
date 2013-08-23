##===- giri/test/matrix_multiply/Makefile -----------------*- Makefile -*-===##

NAME = matrix_multiply-seq
#NAME = matrix_multiply-pthread
GIRI_DIR = ../../build/
LDFLAGS = -pthread
INPUT ?= 16

include ../UnitTests/Makefile.common

clean-all:
	@ rm -f matrix_file_out_*.txt
