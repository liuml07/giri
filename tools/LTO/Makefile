##===- tools/lto/Makefile ----------------------------------*- Makefile -*-===##
# 
#                     The LLVM Compiler Infrastructure
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##

LEVEL = ../..
LIBRARYNAME = LTO

LINK_LIBS_IN_SHARED = 1

#
# Build shared libraries on all platforms except Cygwin and MingW (which do
# not support them).
#
ifneq ($(OS),Cygwin)
ifneq ($(OS),MingW)
SHARED_LIBRARY = 1
endif
endif

LINK_COMPONENTS := $(TARGETS_TO_BUILD) ipo scalaropts linker bitreader bitwriter

USEDLIBS := giri.a dgutility.a

# Include this here so we can get the configuration of the targets
# that have been configured for construction. We have to do this 
# early so we can set up LINK_COMPONENTS before including Makefile.rules
include $(LEVEL)/Makefile.common

LINK_LIBS_IN_SHARED = 1
SHARED_LIBRARY = 1

LINK_COMPONENTS := $(TARGETS_TO_BUILD) ipo scalaropts linker bitreader bitwriter

USEDLIBS := giri.a dgutility.a inv_generate.a

ifeq ($(HOST_OS),Darwin)
    # set dylib internal version number to llvmCore submission number
    ifdef LLVM_SUBMIT_VERSION
        LLVMLibsOptions := $(LLVMLibsOptions) -Wl,-current_version \
                        -Wl,$(LLVM_SUBMIT_VERSION).$(LLVM_SUBMIT_SUBVERSION) \
                        -Wl,-compatibility_version -Wl,1
    endif
    # extra options to override libtool defaults 
    LLVMLibsOptions    := $(LLVMLibsOptions)  \
                         -avoid-version \
                         -Wl,-exported_symbols_list -Wl,$(PROJ_SRC_DIR)/lto.exports \
                         -Wl,-dead_strip \
                         -Wl,-seg1addr -Wl,0xE0000000 

    # Mac OS X 10.4 and earlier tools do not allow a second -install_name on command line
    DARWIN_VERS := $(shell echo $(TARGET_TRIPLE) | sed 's/.*darwin\([0-9]*\).*/\1/')
    ifneq ($(DARWIN_VERS),8)
       LLVMLibsOptions    := $(LLVMLibsOptions)  \
                            -no-undefined -Wl,-install_name \
                            -Wl,"@executable_path/../lib/lib$(LIBRARYNAME)$(SHLIBEXT)"
    endif
endif

#
# These rules create symbolic links from the poolalloc object tree to the
# SAFECode object tree.  This allows the LLVM build machinery to handle
# library dependencies.
#
$(PROJ_OBJ_ROOT)/$(BuildMode)/lib/libpoolalloc.a: $(POOLALLOC_OBJDIR)/$(BuildMode)/lib/libpoolalloc.a
	$(VERB) ln -fs $< $@

$(PROJ_OBJ_ROOT)/$(BuildMode)/lib/libLLVMDataStructure.a: $(POOLALLOC_OBJDIR)/$(BuildMode)/lib/libLLVMDataStructure.a
	$(VERB) ln -fs $< $@

