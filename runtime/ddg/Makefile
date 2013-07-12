##===- projects/sample/lib/sample/Makefile -----------------*- Makefile -*-===##

#
# Indicate where we are relative to the top of the source tree.
#
LEVEL=../..

#
# Give the name of a library.  This will build a dynamic version.
#
LIBRARYNAME=rtinv
BYTECODE_LIBRARY=1
ARCHIVE_LIBRARY=1

#LIBRARYNAME=libddgtrace
#DONT_BUILD_RELINKED=1
#ARCHIVE_LIBRARY=1

#all-local:: Debug/libddgtrace.bc

#Debug :
#	mkdir Debug

#Debug/libddgtrace.bc : libddgtrace.c Debug
#	$(LLVMGCC) -c $< -o $@

#
# Include Makefile.common so we know what to do.
#
include $(LEVEL)/Makefile.common

