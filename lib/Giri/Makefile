##===- giri/lib/Giri/Makefile ----------------------------*- Makefile -*-===##

#
# Indicate where we are relative to the top of the source tree.
#
LEVEL=../..

#
# Give the name of a library.  This will build a dynamic version.
#
LIBRARYNAME=giri

#
# Build a shared library on platforms that support it.
#
ifneq ($(OS),Cygwin)
ifneq ($(OS),MingW)
SHARED_LIBRARY := 1
endif
endif

LLVMLIBS = support

LINK_COMPONENTS := Support

USEDLIBS :=  Support.a

#
# Include Makefile.common so we know what to do.
#
include $(LEVEL)/Makefile.common
