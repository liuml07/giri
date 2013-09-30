##===- giri/test/kmeans/Makefile ---------------------------*- Makefile -*-===##

VERSION ?= pthread
CRITERION_TYPE ?= inst
DEBUGFLAGS ?= -debug
INPUT ?= 16

###### Don't modify the following unless you know exactly what you'r doing #####
ifeq (pthread,$(VERSION))
NAME = kmeans-pthread
LDFLAGS = -lpthread
else
NAME = kmeans-seq
VERSION = seq
endif

GIRI_DIR = ../../build/
SRC_FILES = $(NAME).c
CRITERION = "-criterion-$(CRITERION_TYPE)=criterion-$(CRITERION_TYPE)-$(VERSION).txt"
TEST_ANS = ans-$(CRITERION_TYPE)-$(VERSION).txt

include ../Makefile.common
