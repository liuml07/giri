##===- giri/test/kmeans/Makefile ---------------------------*- Makefile -*-===##

ifeq (pthread,$(VERSION))
NAME = kmeans-pthread
LDFLAGS = -lpthread
else
NAME = kmeans-seq
endif
GIRI_DIR = ../../build/

include ../UnitTests/Makefile.common
