##===- diagnosis/test/TEST.tamashigiri.Makefile ------------*- Makefile -*-===##
#
# This test runs both SAFECode and valgrind on all of the Programs and produces
# performance numbers and statistics.
#
##===----------------------------------------------------------------------===##

include $(PROJ_OBJ_ROOT)/Makefile.common

#
# Turn on debug information for use with the SAFECode tool
#
CFLAGS = -g -O2 -fno-strict-aliasing

CURDIR  := $(shell cd .; pwd)
PROGDIR := $(shell cd $(LLVM_SRC_ROOT)/projects/test-suite; pwd)/
RELDIR  := $(subst $(PROGDIR),,$(CURDIR))
GCCLD    = $(LLVM_OBJ_ROOT)/$(CONFIGURATION)/bin/gccld

# Pool allocator pass shared object
PA_SO    := $(PROJECT_DIR)/Debug/lib/libgiri.a

# Pool allocator runtime library
PA_RT_O  := $(PROJECT_DIR)/$(CONFIGURATION)/lib/librtgiri.a
#PA_RT_BC := librtgiri.bca
#PA_RT_BC := $(addprefix $(PROJECT_DIR)/$(CONFIGURATION)/lib/, $(PA_RT_BC))

# Pool System library for interacting with the system
#POOLSYSTEM := $(PROJECT_DIR)/$(CONFIGURATION)/lib/UserPoolSystem.o
POOLSYSTEM :=

#
# Extra command line options for opt that run the transforms for gathering
# dynamic tracing information
#
TRACEOPTS := -load $(PROJECT_DIR)/$(CONFIGURATION)/lib/libdgutility$(SHLIBEXT) \
             -load $(PROJECT_DIR)/$(CONFIGURATION)/lib/libgiri$(SHLIBEXT) \
             -bbnum -trace-giri -remove-bbnum \
             -t Output/bbrecord
TRACER := $(LLVM_OBJ_ROOT)/projects/diagnosis/$(CONFIGURATION)/bin/tracer -trace -t Output/bbrecord

OPTZN_PASSES := -strip-debug -std-compile-opts


#
# This rule runs the transform to add tracing instrumentation to a program.
#
$(PROGRAMS_TO_TEST:%=Output/%.trace.bc): \
Output/%.trace.bc: Output/%.llvm.bc $(PA_SO) $(LOPT)
	-@rm -f $(CURDIR)/$@.info
	-$(TRACER) -stats -info-output-file=$(CURDIR)/$@.info $< -f -o $@ 2>&1 >> $@.out

#
# These rules compile the new .bc file into a .c file using llc
#
$(PROGRAMS_TO_TEST:%=Output/%.trace.s): \
Output/%.trace.s: Output/%.trace.bc $(LLC)
	-$(LLC) $(LLCFLAGS) -f $< -o $@

$(PROGRAMS_TO_TEST:%=Output/%.trace.cbe.c): \
Output/%.trace.cbe.c: Output/%.trace.bc $(LLC)
	-$(LLC) -march=c $(LLCFLAGS) -f $< -o $@

#
# These rules compile the CBE .c file into a final executable
#
ifdef SC_USECBE
$(PROGRAMS_TO_TEST:%=Output/%.trace): \
Output/%.trace: Output/%.trace.cbe.c $(PA_RT_O) $(POOLSYSTEM)
	-$(LLVMGCC) $(CFLAGS) $< $(LLCLIBS) $(PA_RT_O) $(POOLSYSTEM) $(LDFLAGS) -o $@ -static -lstdc++
else
$(PROGRAMS_TO_TEST:%=Output/%.trace): \
Output/%.trace: Output/%.trace.s $(PA_RT_O) $(POOLSYSTEM)
	-$(LLVMGCC) $(CFLAGS) $< $(LLCLIBS) $(PA_RT_O) $(POOLSYSTEM) $(LDFLAGS) -o $@ -static -lstdc++
endif

##############################################################################
# Rules for running executables and generating reports
##############################################################################

ifndef PROGRAMS_HAVE_CUSTOM_RUN_RULES

#
# This rule runs the generated executable, generating timing information, for
# normal test programs
#
$(PROGRAMS_TO_TEST:%=Output/%.trace.out-llc): \
Output/%.trace.out-llc: Output/%.trace
	-$(RUNSAFELY) $(STDIN_FILENAME) $@ $< $(RUN_OPTIONS)

else

#
# This rule runs the generated executable, generating timing information, for
# SPEC
#
$(PROGRAMS_TO_TEST:%=Output/%.trace.out-llc): \
Output/%.trace.out-llc: Output/%.trace
	-$(SPEC_SANDBOX) tracecbe-$(RUN_TYPE) $@ $(REF_IN_DIR) \
             $(RUNSAFELY) $(STDIN_FILENAME) $(STDOUT_FILENAME) \
                  ../../$< $(RUN_OPTIONS)
	-(cd Output/tracecbe-$(RUN_TYPE); cat $(LOCAL_OUTPUTS)) > $@
	-cp Output/tracecbe-$(RUN_TYPE)/$(STDOUT_FILENAME).time $@.time

endif


# This rule diffs the post-poolallocated version to make sure we didn't break
# the program!
$(PROGRAMS_TO_TEST:%=Output/%.trace.diff-llc): \
Output/%.trace.diff-llc: Output/%.out-nat Output/%.trace.out-llc
	@cp Output/$*.out-nat Output/$*.trace.out-nat
	-$(DIFFPROG) llc $*.trace $(HIDEDIFF)

# This rule wraps everything together to build the actual output the report is
# generated from.
$(PROGRAMS_TO_TEST:%=Output/%.$(TEST).report.txt): \
Output/%.$(TEST).report.txt: Output/%.out-nat                \
                             Output/%.trace.diff-llc     \
                             Output/%.LOC.txt
	@echo > $@
	@-if test -f Output/$*.out-nat; then \
	  printf "GCC-RUN-TIME: " >> $@;\
	  grep "^program" Output/$*.out-nat.time >> $@;\
        fi
	@-if test -f Output/$*.trace.diff-llc; then \
	  printf "CBE-RUN-TIME-SAFECODE: " >> $@;\
	  grep "^program" Output/$*.trace.out-llc.time >> $@;\
	fi
	printf "LOC: " >> $@
	cat Output/$*.LOC.txt >> $@
	#@cat Output/$*.$(TEST).bc.info >> $@

$(PROGRAMS_TO_TEST:%=test.$(TEST).%): \
test.$(TEST).%: Output/%.$(TEST).report.txt
	@echo "---------------------------------------------------------------"
	@echo ">>> ========= '$(RELDIR)/$*' Program"
	@echo "---------------------------------------------------------------"
	@cat $<

REPORT_DEPENDENCIES := $(PA_RT_O) $(PA_SO) $(PROGRAMS_TO_TEST:%=Output/%.llvm.bc) $(LLC) $(LOPT)
