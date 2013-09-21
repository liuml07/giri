##===- giri/test/histogram/Makefile -----------------*- Makefile -*-===##

ifeq (pthread,$(VERSION))
NAME = histogram-pthread
LDFLAGS = -lpthread
else
NAME = histogram-seq
VERSION = seq
endif

GIRI_DIR = ../../build/
SRC_FILES = $(NAME).c
CRITERION ?= "-criterion-loc=loc-$(VERSION).txt"
#CRITERION ?= "-criterion-inst=inst-$(VERSION).txt"
INPUT ?= 16

include ../UnitTests/Makefile.common
