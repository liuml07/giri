##===- runtime/Giri/Makefile -------------------------------*- Makefile -*-===##

# Indicate where we are relative to the top of the source tree.
LEVEL = ../..

# Give the name of a library.  This will build a dynamic version.
LIBRARYNAME = rtgiri

ifneq ($(strip $(LLVMCC)),)
BYTECODE_LIBRARY = 1
endif

ARCHIVE_LIBRARY = 1

include $(LEVEL)/Makefile.common
