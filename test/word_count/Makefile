##===- giri/test/word_count/Makefile -----------------------*- Makefile -*-===##

TEST_PARALLELISM ?= pthread
#CRITERION ?= "-criterion-loc=loc-$(TEST_PARALLELISM).txt"
CRITERION ?= "-criterion-inst=inst-$(TEST_PARALLELISM).txt"
DEBUGFLAGS ?= -debug
INPUT ?= Makefile

##### Don't modify the following unless you know exactly what you're doing #####
ifeq (pthread,$(TEST_PARALLELISM))
NAME = word_count-pthread
SRC_FILES = word_count-pthread.c sort-pthread.c
LDFLAGS = -lpthread
else
NAME = word_count-seq
SRC_FILES = word_count-seq.c
TEST_PARALLELISM = seq
endif

GIRI_DIR = ../../build/

include ../Makefile.common

clean-all:
	@ rm -f matrix_file_out_*.txt
