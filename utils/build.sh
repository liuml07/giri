#!/bin/bash

LLVMSRC_DIR=$(cd .. && pwd)/llvm
LLVMOBJ_DIR=$LLVMSRC_DIR/build

mkdir build
cd build
../configure --with-llvmsrc=$LLVMSRC_DIR --with-llvmobj=$LLVMOBJ_DIR --enable-optimized
make
cd ..
make test
