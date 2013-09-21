##===- giri/test/linear_regression/Makefile -----------------*- Makefile -*-===##

VERSION ?= pthread
CRITERION ?= "-criterion-loc=loc-$(VERSION).txt"
#CRITERION ?= "-criterion-inst=inst-$(VERSION).txt"
DEBUGFLAGS ?= -debug
INPUT ?= 4

###### Don't modify the following unless you know exactly what you'r doing #####
ifeq (pthread,$(VERSION))
NAME = linear_regression-pthread
LDFLAGS = -lpthread
else
NAME = linear_regression-seq
VERSION = seq
endif

GIRI_DIR = ../../build/
SRC_FILES = $(NAME).c

include ../UnitTests/Makefile.common

clean-all:
	@ rm -f matrix_file_out_*.txt
