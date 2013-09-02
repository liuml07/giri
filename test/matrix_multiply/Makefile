##===- giri/test/matrix_multiply/Makefile -----------------*- Makefile -*-===##

VERSION ?= pthread

ifeq (pthread,$(VERSION))
NAME = matrix_multiply-pthread
LDFLAGS = -lpthread
else
NAME = matrix_multiply-seq
VERSION = seq
endif

GIRI_DIR = ../../build/
SRC_FILES = $(NAME).c
#CRITERION ?= "-criterion-loc=loc-$(VERSION).txt"
CRITERION ?= "-criterion-inst=inst-$(VERSION).txt"
INPUT ?= 16
DEBUGFLAGS ?= -debug-only=giri

include ../UnitTests/Makefile.common

clean-all:
	@ rm -f matrix_file_out_*.txt
