##===- giri/test/Makefile ------------------------------*- Makefile -*-===##
#
# Define the default test rule, which allows people to just type 'make test' in
# this directory.
#
##===------------------------------------------------------------------===##

# The default target in this directory is the test:: target
test::

LEVEL = ..
include $(LEVEL)/Makefile.common

ifndef NO_STABLE_NUMBERS
STABLERUN := GET_STABLE_NUMBERS=1
else
STABLERUN :=
endif

ifndef NO_LARGE_SIZE
LARGESIZE := LARGE_PROBLEM_SIZE=1
else
LARGESIZE :=
endif

ifdef ENABLE_OPTIMIZED
OPTIMIZED := ENABLE_OPTIMIZED=1
else
OPTIMIZED :=
endif

RUNTIMELIMIT=3600

PROFILING_DAT_DIR := $(shell pwd)/data
PROFILING_RESULT_DIR := $(shell pwd)/data/profiling

LARGE_PROBLEM_SIZE_DIRS :=


LARGE_PROBLEM_SIZE_DIRS := \
   $(addprefix $(TEST_OBJDIR)/,$(LARGE_PROBLEM_SIZE_DIRS))

#DEBUG_SAFECODE := DEBUG_SAFECODE=1

NORMAL_PROBLEM_SIZE_DIRS := \
  MultiSource/Benchmarks/Olden/bh \
  #MultiSource/Benchmarks/Olden/bisort \
  #MultiSource/Benchmarks/Olden/mst \
  #MultiSource/Benchmarks/Olden/perimeter \
  #MultiSource/Benchmarks/Olden/power \
  #MultiSource/Benchmarks/Olden/treeadd \
  #MultiSource/Benchmarks/Olden/tsp \
  #External/Namd \
  #External/SPEC/CINT2000 \
  #External/Povray \
  #MultiSource/Benchmarks/Olden \
  #MultiSource/Benchmarks/Olden/health \
  #External/SPEC/CINT2000/176.gcc \
  #External/SPEC/CINT2000/175.vpr \
  #MultiSource/Benchmarks/Olden \
  #External/SPEC/CINT2000/197.parser \
  #External/SPEC/CINT2000/254.gap \
  #External/SPEC/CINT2000/300.twolf \
  #External/SPEC/CINT2000 \
  #External/SPEC/CINT2000/252.eon \
  #MultiSource/Applications/Burg \
#  MultiSource/Benchmarks/Olden \
#  MultiSource/Benchmarks/Olden/em3d \
#  MultiSource/Benchmarks/Olden/voronoi \
#   MultiSource/Benchmarks/Olden \
#   MultiSource/Benchmarks/Ptrdist \
#   External/SPEC/CINT2000/164.gzip \
#   SingleSource \
#   External/SPEC/CINT2000/300.twolf \
#   SingleSource \
#   MultiSource \
#   MultiSource/Benchmarks \
#   External/SPEC/CINT2000 \
#   External/SPEC/CINT2000/181.mcf \
#   External/SPEC/CINT2000/256.bzip2 \
   #External/SPEC/CINT2000/186.crafty \
   #External/SPEC/CFP2000/188.ammp \
   #MultiSource/Applications/ghttpd \
   #MultiSource/Benchmarks/Olden/bh \
   #MultiSource/Benchmarks/Ptrdist \
   #MultiSource/Applications \
   #MultiSource/Benchmarks


NORMAL_PROBLEM_SIZE_DIRS := \
   $(addprefix $(TEST_OBJDIR)/,$(NORMAL_PROBLEM_SIZE_DIRS))

##############################################################################
# Targets for gathering statistics for programs for papers
##############################################################################
               
# Program tests for Dynamic Slicing
giritest::
	for dir in $(LARGE_PROBLEM_SIZE_DIRS); do \
            (cd $$dir; \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) TEST=tamashigiri \
               POOLALLOC_OBJDIR=$(POOLALLOC_OBJDIR) \
               RUNTIMELIMIT=$(RUNTIMELIMIT) \
               LARGE_PROBLEM_SIZE=1 \
               report.html report.csv) \
        done
	for dir in $(NORMAL_PROBLEM_SIZE_DIRS); do \
	    (cd $$dir; \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) TEST=tamashigiri \
               POOLALLOC_OBJDIR=$(POOLALLOC_OBJDIR) \
               RUNTIMELIMIT=$(RUNTIMELIMIT) $(DEBUG_SAFECODE) \
               $(LARGESIZE) $(STABLERUN) $(OPTIMIZED) \
                   report.html report.csv; \
      ) \
        done
	@for dir in $(LARGE_PROBLEM_SIZE_DIRS); do \
            (cd $$dir;  \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -s TEST=tamashigiri \
                   LARGE_PROBLEM_SIZE=1 report) \
        done
	@for dir in $(NORMAL_PROBLEM_SIZE_DIRS); do \
	    (cd $$dir; \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -s TEST=tamashigiri \
                   report) \
        done
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"

##############################################################################
# Targets for running tests and gathering statistics for arbitrary tests
##############################################################################

# test target - Descend into projects/llvm-test and run the TEST.safecode.Makefile
# tests...
test::
	(cd $(TEST_OBJDIR)/$(SUBDIR); \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -j1 TEST=safecode \
                   $(LARGESIZE) $(STABLERUN) report report.html)
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"

pacompiletime::
	(cd $(TEST_OBJDIR)/$(SUBDIR); \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -j1 TEST=pacompiletime \
                   report report.html)
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"

ptrcomp::
	(cd $(TEST_OBJDIR)/$(SUBDIR); \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -j1 TEST=ptrcomp \
                   $(LARGESIZE) $(STABLERUN) report report.html)
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"

ptrcomptest::
	(cd $(TEST_OBJDIR)/$(SUBDIR); \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -j1 TEST=ptrcomp \
                   $(LARGESIZE) report)
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"

vtl::
	(cd $(TEST_OBJDIR)/$(SUBDIR); \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -j1 TEST=pavtl \
                   test report report.csv)
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"

perf::
	(cd $(TEST_OBJDIR)/$(SUBDIR); \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -j1 TEST=perf \
                   test report report.csv)
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"

optzn::
	(cd $(TEST_OBJDIR)/$(SUBDIR); \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -j1 TEST=optzn \
                   $(LARGESIZE) $(STABLERUN) test report report.csv)
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"

p4perf::
	(cd $(TEST_OBJDIR)/$(SUBDIR); \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -j1 TEST=p4perf \
                   test report report.csv)
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"

strace::
	(cd $(TEST_OBJDIR)/$(SUBDIR); \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -j1 TEST=strace \
                   test)
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"

cputrack::
	(cd $(TEST_OBJDIR)/$(SUBDIR); \
               PROJECT_DIR=$(PROJ_OBJ_ROOT) $(MAKE) -j1 TEST=cputrack \
                   report report.csv)
	@printf "\a"; sleep 1; printf "\a"; sleep 1; printf "\a"

