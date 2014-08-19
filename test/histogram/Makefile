##===- giri/test/histogram/Makefile -----------------*- Makefile -*-===##

ifeq (pthread,$(TEST_PARALLELISM))
NAME = histogram-pthread
LDFLAGS = -lpthread
else
NAME = histogram-seq
TEST_PARALLELISM = seq
endif

GIRI_DIR = ../../build/
SRC_FILES = $(NAME).c
CRITERION ?= "-criterion-loc=loc-$(TEST_PARALLELISM).txt"
#CRITERION ?= "-criterion-inst=inst-$(TEST_PARALLELISM).txt"
INPUT ?= 16

include ../Makefile.common
