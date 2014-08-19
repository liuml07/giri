##===- giri/test/linear_regression/Makefile -----------------*- Makefile -*-===##

TEST_PARALLELISM ?= pthread
CRITERION ?= "-criterion-loc=loc-$(TEST_PARALLELISM).txt"
#CRITERION ?= "-criterion-inst=inst-$(TEST_PARALLELISM).txt"
DEBUGFLAGS ?= -debug
INPUT ?= 4

###### Don't modify the following unless you know exactly what you'r doing #####
ifeq (pthread,$(TEST_PARALLELISM))
NAME = linear_regression-pthread
LDFLAGS = -lpthread
else
NAME = linear_regression-seq
TEST_PARALLELISM = seq
endif

GIRI_DIR = ../../build/
SRC_FILES = $(NAME).c

include ../Makefile.common

clean-all:
	@ rm -f matrix_file_out_*.txt
