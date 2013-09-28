##===- giri/test/Makefile ----------------------------------*- Makefile -*-===##
#
# Define the default test rule. Just type 'make test' in this directory.
#
##===----------------------------------------------------------------------===##

.PHONY: test lib clean

# The default target in this directory is the test:: target
test:: lib
	@ echo "Running the test cases, fases and applications..."
	@ for t in UnitTests/test1 UnitTests/test2 UnitTests/test3 UnitTests/test4 UnitTests/test5 UnitTests/test8 UnitTests/test9 UnitTests/test10 UnitTests/test11 UnitTests/test12 UnitTests/test13 UnitTests/test14 UnitTests/test15 UnitTests/test16 UnitTests/test17 UnitTests/test18 UnitTests/test19 UnitTests/test20 matrix_multiply pca kmeans;\
		do\
			printf "\033[37m%-20s" "$$t";\
			make clean -s -C $$t > /dev/null 2>&1;\
			make -s -C $$t > /dev/null 2>&1;\
			make test -s -C $$t DEBUGFLAGS= > /dev/null 2>&1;\
			if [ $$? -eq 0 ]; then\
				printf "\033[34m[PASS]\n";\
			else\
				printf "\033[31m[FAIL]\n";\
			fi;\
			make clean -s -C $$t > /dev/null 2>&1;\
		done
	
lib:
	@ echo -n "Building the Giri..."
	@ $(MAKE) -s -C ../build 2>&1 > /dev/null || (echo "Fail to build the Giri lib!" && exit 1)
	@ echo

clean:
	@ for u in $(shell ls UnitTests); do \
		[[ -d UnitTests/$$u ]] && make clean -C UnitTests/$$u;\
		done
	@ $(MAKE) clean -C kmeans
	@ $(MAKE) clean -C matrix_multiply
	@ $(MAKE) clean -C pca
