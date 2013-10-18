#===- tools/Tracer/Makefile --------------------------------*- Makefile -*-===##
# 
#                     The LLVM Compiler Infrastructure
#
# This file was developed by the LLVM research group and is distributed under
# the University of Illinois Open Source License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##

LEVEL = ../..
TOOLNAME = tracer
CXXFLAGS += -fno-rtti
REQUIRES_EH := 1

LINK_COMPONENTS := bitreader bitwriter instrumentation scalaropts ipo nativecodegen

USEDLIBS := giri.a dgutility.a

include $(LEVEL)/Makefile.common
