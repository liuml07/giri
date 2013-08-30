##===- giri/test/matrix_multiply/Makefile -----------------*- Makefile -*-===##

ifeq (pthread,$(VERSION))
NAME = pca-pthread
LDFLAGS = -lpthread
else
NAME = pca-seq
VERSION = seq
endif

GIRI_DIR = ../../build/
SRC_FILES = $(NAME).c
CRITERION ?= "-criterion-loc=loc-$(VERSION).txt"
#CRITERION ?= "-criterion-inst=inst-$(VERSION).txt"
INPUT ?= 16

include ../UnitTests/Makefile.common
