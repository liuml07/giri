##===- giri/test/word_count/Makefile -----------------------*- Makefile -*-===##

VERSION ?= pthread
#CRITERION ?= "-criterion-loc=loc-$(VERSION).txt"
CRITERION ?= "-criterion-inst=inst-$(VERSION).txt"
DEBUGFLAGS ?= -debug
INPUT ?= Makefile

##### Don't modify the following unless you know exactly what you're doing #####
ifeq (pthread,$(VERSION))
NAME = word_count-pthread
SRC_FILES = word_count-pthread.c sort-pthread.c
LDFLAGS = -lpthread
else
NAME = word_count-seq
SRC_FILES = word_count-seq.c
VERSION = seq
endif

GIRI_DIR = ../../build/

include ../Makefile.common

clean-all:
	@ rm -f matrix_file_out_*.txt
