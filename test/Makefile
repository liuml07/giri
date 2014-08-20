##===- giri/test/Makefile ----------------------------------*- Makefile -*-===##
#
# Define the default test rule. Just type 'make test' in this directory.
#
##===----------------------------------------------------------------------===##

.PHONY: test lib clean

TEST_LOG ?= /dev/null

# The default target in this directory is the test:: target
test:: lib
	@ echo "Running the test cases..."
	@ RET=0;\
	  for t in $$(cat auto-tests.txt);\
	  do\
	      printf "\033[37m%-20s" "$$t";\
	  	  make clean -s -C $$t > /dev/null 2>&1;\
		  make -s -C $$t > /dev/null 2>&1;\
		  make test -s -C $$t DEBUGFLAGS= >> $(TEST_LOG) 2>&1;\
		  if [ $$? -eq 0 ]; then\
		  	  printf "\033[34m[PASS]\033[0m\n";\
		  else\
		      printf "\033[31m[FAIL]\033[0m\n";\
		      RET=1;\
		  fi;\
		  make clean -s -C $$t > /dev/null 2>&1;\
	  done;\
	  [ "$(TEST_LOG)" != "/dev/null" ] && cat $(TEST_LOG);\
	  exit $$RET

lib:
	@ echo -n "Building the Giri..."
	@ $(MAKE) -s -C ../build 2>&1 > /dev/null || (echo "Fail to build the Giri lib!" && exit 1)
	@ echo

clean:
	@ for t in $$(cat auto-tests.txt); do make clean -C $$t; done
	@ rm -f $(TEST_LOG)
