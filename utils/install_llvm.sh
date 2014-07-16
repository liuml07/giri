#!/bin/bash

if [ $# -lt 1 ]; then
        echo "Pleas set the version of LLVM to build!"
        exit 1
else
        VERSION=$1
fi

case $VERSION in
        "3.1")
                ;;
        "3.4")
                wget http://llvm.org/releases/3.4/clang-3.4.src.tar.gz
                wget http://llvm.org/releases/3.4/llvm-3.4.src.tar.gz
                wget http://llvm.org/releases/3.4/compiler-rt-3.4.src.tar.gz
                tar xf llvm-3.4.src.tar.gz
                tar xf clang-3.4.src.tar.gz
                tar xf compiler-rt-3.4.src.tar.gz
                mv llvm-3.4 llvm
                mv clang-3.4 llvm/tools/clang
                mv compiler-rt-3.4 llvm/projects/compiler-rt
                mkdir -p llvm/build
                cd llvm/build
                ../configure --enable-optimized
                make
                sudo make install
                ;;
esac
