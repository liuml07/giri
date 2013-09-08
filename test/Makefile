##===- giri/test/Makefile ---------------------------------*- Makefile -*-===##
#
# Define the default test rule. Just type 'make test' in this directory.
#
##===-----------------------000-------------------------------------------===##

# The default target in this directory is the test:: target
test::
	@ echo Nothing to be done yet.

clean:
	@for u in $(shell ls UnitTests); do \
		[[ -d UnitTests/$$u ]] && make clean -C UnitTests/$$u;\
		done
	@ $(MAKE) clean -C kmeans
	@ $(MAKE) clean -C matrix_multiply
	@ $(MAKE) clean -C pca
