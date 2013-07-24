##===- giri/lib/Utility/Makefile --------------------------*- Makefile -*-===##

#
# Indicate where we are relative to the top of the source tree.
#
LEVEL=../..

#
# Give the name of a library.  This will build a dynamic version.
#
LIBRARYNAME=dgutility

#
# Build a shared library on platforms that support it.
#
ifneq ($(OS),Cygwin)
ifneq ($(OS),MingW)
SHARED_LIBRARY := 1
endif
endif

#
# Include Makefile.common so we know what to do.
#
include $(LEVEL)/Makefile.common
