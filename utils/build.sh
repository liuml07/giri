#!/bin/bash

LLVMSRC_DIR=llvm
LLVMOBJ_DIR=llvm/build

mkdir build
cd build
../configure --with-llvmsrc=$LLVMSRC_DIR --with-llvmobj=$LLVMOBJ_DIR --enable-optimized
make
cd ..
make test
