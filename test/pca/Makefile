##===- giri/test/matrix_multiply/Makefile -----------------*- Makefile -*-===##

ifeq (pthread,$(VERSION))
NAME = pca-pthread
LDFLAGS = -lpthread
else
NAME = pca-seq
endif

GIRI_DIR = ../../build/
INPUT ?= 16

include ../UnitTests/Makefile.common
