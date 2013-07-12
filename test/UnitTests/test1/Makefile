

NAME = extlibcalls

#VERSION = Release+Asserts
VERSION = Debug+Asserts
#OBJ_TYPE = objWithGcc
OBJ_TYPE = buildDebug
#OBJ_PROJ_DIR = /home/ssahoo2/research/sw-bug-diagnosis/llvm-3.1-dup/$(OBJ_TYPE)/projects/diagnosis
OBJ_PROJ_DIR = /home/ssahoo2/sw-bug-diagnosis/llvm-3.1/$(OBJ_TYPE)/projects/diagnosis
LIB_PROJ_DIR = $(OBJ_PROJ_DIR)/$(VERSION)/lib
BIN_PROJ_DIR = $(OBJ_PROJ_DIR)/$(VERSION)/bin

CCOMPILER = clang

CFLAGS +=  -g -O0 -c -emit-llvm 

INCLUDE_DIR   +=  

#Do Not run any optimizations for testing
OPTFLAGS +=  -simplifycfg -mem2reg -scalarrepl -instcombine -raiseallocs -simplifycfg -mem2reg -globalopt -globaldce -ipsccp -deadargelim -instcombine -simplifycfg -prune-eh -functionattrs -argpromotion -simplify-libcalls -jump-threading -simplifycfg -scalarrepl -instcombine -condprop -tailcallelim -simplifycfg -reassociate -loop-rotate -loop-unswitch -instcombine -loop-deletion -instcombine -memcpyopt -sccp -instcombine -condprop -dse -adce -simplifycfg -strip-dead-prototypes -deadtypeelim -constmerge -condprop -dse -globaldce -instcombine -ipsccp -mergereturn 


$(NAME).inv.out : $(NAME).inv.s $(OBJ_PROJ_DIR)/runtime/giri/$(VERSION)/Tracing.o $(OBJ_PROJ_DIR)/runtime/ddg/$(VERSION)/libddgtrace_debug.o
	$(CCOMPILER) -fno-strict-aliasing $+ -o $@ -lpthread

$(NAME).inv.s : $(NAME).inv.bc 
	llc -asm-verbose=false -O0 $< -o $@

$(NAME).inv.bc : $(NAME).bc $(LIB_PROJ_DIR)/libdgutility.so $(LIB_PROJ_DIR)/libinv_generate.so $(LIB_PROJ_DIR)/libgiri.so
	time $(OBJ_PROJ_DIR)/../../$(VERSION)/bin/opt -load $(LIB_PROJ_DIR)/libdgutility.so -load $(LIB_PROJ_DIR)/libgiri.so -load $(LIB_PROJ_DIR)/libinv_generate.so -mergereturn -bbnum -lsnum -trace-giri -t bbrecord -inv-gen -remove-bbnum -remove-lsnum -debug -stats $< -f -o $@


#./extlibcalls.inv.out SWARUP ANUP


$(NAME).trace.out : $(NAME).trace.s $(OBJ_PROJ_DIR)/runtime/giri/$(VERSION)/Tracing.o $(OBJ_PROJ_DIR)/runtime/ddg/$(VERSION)/libddgtrace_debug.o
	$(CCOMPILER) -fno-strict-aliasing $+ -o $@ -lpthread

$(NAME).trace.s : $(NAME).trace.bc 
	llc -asm-verbose=false -O0 $< -o $@

$(NAME).trace.bc : $(NAME).bc $(LIB_PROJ_DIR)/libdgutility.so $(LIB_PROJ_DIR)/libinv_generate.so $(LIB_PROJ_DIR)/libgiri.so
	time $(OBJ_PROJ_DIR)/../../$(VERSION)/bin/opt -load $(LIB_PROJ_DIR)/libdgutility.so -load $(LIB_PROJ_DIR)/libgiri.so -load $(LIB_PROJ_DIR)/libinv_generate.so -mergereturn -bbnum -lsnum -trace-giri -t bbrecord -inv-insert -remove-bbnum -remove-lsnum -debug -stats $< -f -o $@


#./extlibcalls.trace.out DHARITRI MINA


$(NAME).slice.txt :  $(NAME).bc $(LIB_PROJ_DIR)/libdgutility.so $(LIB_PROJ_DIR)/libinv_generate.so $(LIB_PROJ_DIR)/libgiri.so
	time $(OBJ_PROJ_DIR)/../../$(VERSION)/bin/opt -load $(LIB_PROJ_DIR)/libdgutility.so -load $(LIB_PROJ_DIR)/libgiri.so -load $(LIB_PROJ_DIR)/libinv_generate.so -bbnum -lsnum -dgiri -tf bbrecord -dfs=true -inv-filter -trace-cd=true -remove-bbnum -remove-lsnum -select-origin-as-main -stats $<  -f -o  /dev/null  >& $@

#./extlibcalls.slice.txt


$(NAME).bc : $(NAME).c 	
	$(CCOMPILER) $(CFLAGS) $+ -o $@

%.ll : %.bc
	llvm-dis < $< > $@



clean:
	rm -f *.ll *.bc *.exe *.out *.o *.s
	rm -f core*
	rm -f Invariants.txt bbrecord







#opt -load /home/ssahoo2/research/sw-bug-diagnosis/llvm-3.1-dup/obj/projects/diagnosis/Release+Asserts/lib/libdgutility.so -load /home/ssahoo2/research/sw-bug-diagnosis/llvm-3.1-dup/obj/projects/diagnosis/Release+Asserts/lib/libgiri.so -load /home/ssahoo2/research/sw-bug-diagnosis/llvm-3.1-dup/obj/projects/diagnosis/Release+Asserts/lib/libinv_generate.so -mergereturn -bbnum -lsnum -test-giri -tf bbrecord -dfs=false -trace-cd=true -remove-bbnum -remove-lsnum -select-origin-as-main -debug -stats extlibcalls.bc -f -o  /dev/null

#/home/ssahoo2/research/sw-bug-diagnosis/llvm-3.1-dup/obj/projects/diagnosis/Release+Asserts/bin/prtrace bbrecord

#opt -load /home/ssahoo2/research/sw-bug-diagnosis/llvm-3.1-dup/obj/projects/diagnosis/Release+Asserts/lib/libdgutility.so -load /home/ssahoo2/research/sw-bug-diagnosis/llvm-3.1-dup/obj/projects/diagnosis/Release+Asserts/lib/libgiri.so -load /home/ssahoo2/research/sw-bug-diagnosis/llvm-3.1-dup/obj/projects/diagnosis/Release+Asserts/lib/libinv_generate.so -mergereturn -bbnum -lsnum -dgiri -tf bbrecord -dfs=false -trace-cd=true -remove-bbnum -remove-lsnum -select-origin-as-main -debug -stats ld-temp.o1.bc  -f -o  /dev/null
