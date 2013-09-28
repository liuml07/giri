##===- giri/test/matrix_multiply/Makefile -----------------*- Makefile -*-===##

VERSION ?= pthread
CRITERION_TYPE ?= inst
DEBUGFLAGS ?= -debug
INPUT ?= 4

###### Don't modify the following unless you know exactly what you'r doing #####
ifeq (pthread,$(VERSION))
NAME = matrix_multiply-pthread
LDFLAGS = -lpthread
else
NAME = matrix_multiply-seq
VERSION = seq
endif

GIRI_DIR = ../../build/
SRC_FILES = $(NAME).c
CRITERION = "-criterion-$(CRITERION_TYPE)=criterion-$(CRITERION_TYPE)-$(VERSION).txt"
TEST_ANS = ans-$(CRITERION_TYPE)-$(VERSION).txt

include ../Makefile.common

clean-all:
	@ rm -f matrix_file_out_*.txt
