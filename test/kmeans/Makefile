##===- giri/test/matrix_multiply/Makefile -----------------*- Makefile -*-===##

ifeq (pthread,$(VERSION))
NAME = kmeans-pthread
LDFLAGS = -lpthread
else
NAME = kmeans-seq
VERSION = seq
endif

GIRI_DIR = ../../build/
SRC_FILES = $(NAME).c
CRITERION ?= "-criterion-loc=loc-$(VERSION).txt"
#CRITERION ?= "-criterion-inst=inst-$(VERSION).txt"

include ../UnitTests/Makefile.common
