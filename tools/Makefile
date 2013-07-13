LEVEL = ..

DIRS = PrintTrace Tracer Tarantula

#
# Build libLTO on all platforms except Cygwin and MingW
#
ifneq ($(OS),Cygwin)
ifneq ($(OS),MingW)
DIRS += LTO
endif
endif

include $(LEVEL)/Makefile.common

