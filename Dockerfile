FROM ubuntu

MAINTAINER Mingliang Liu <liuml07@gmail.com>

RUN apt-get update
RUN apt-get upgrade -y
RUN apt-get install -qq -y wget git g++ make python 

ADD . giri

RUN giri/utils/install_llvm.sh 3.4

RUN cd giri/ && utils/build.sh
