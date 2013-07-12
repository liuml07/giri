##===- projects/sample/lib/sample/Makefile -----------------*- Makefile -*-===##

#
# Indicate where we are relative to the top of the source tree.
#
LEVEL=../..

#
# Give the name of a library.  This will build a dynamic version.
#
LIBRARYNAME=inv_generate

#
# Build a shared library on platforms that support it.
#
ifneq ($(OS),Cygwin)
ifneq ($(OS),MingW)
SHARED_LIBRARY := 1
endif
endif

#LINK_LIBS_IN_SHARED = 1

#LINK_COMPONENTS := $(TARGETS_TO_BUILD) ipo scalaropts linker bitreader bitwriter

USEDLIBS := dgutility.so

#
# Include Makefile.common so we know what to do.
#
include $(LEVEL)/Makefile.common
