FROM ubuntu

MAINTAINER Mingliang Liu <liuml07@gmail.com>

ENV LLVM_HOME /usr/local/llvm
ENV BuildMode Release+Asserts
ENV TEST_PARALLELISM seq

RUN apt-get update
RUN apt-get upgrade -y
RUN apt-get install -qq -y wget make g++ python zip unzip autoconf libtool automake

ADD . giri

RUN giri/utils/install_llvm.sh 3.4

RUN cd giri/ && utils/build.sh
