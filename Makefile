##===- giri/Makefile ----------------------------*- Makefile -*-===##
#
# This is the top Makefile for Giri project, dynamic slicing in LLVM
#
##===-----------------------------------------------------------===##

#
# Indicates our relative path to the top of the project's root directory.
#
LEVEL = .
DIRS = lib tools runtime
EXTRA_DIST = include

#
# Include the Master Makefile that knows how to build all.
#
include $(LEVEL)/Makefile.common

