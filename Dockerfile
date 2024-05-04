# Use Ubuntu 20.04 LTS as the base image
FROM ubuntu:20.04

# Avoid prompts from apt-get
ARG DEBIAN_FRONTEND=noninteractive

# Update apt and install necessary packages
RUN apt-get update && \
    apt-get install -y software-properties-common && \
    add-apt-repository ppa:ubuntu-toolchain-r/test -y && \
    apt-get update && \
    apt-get install -y cmake libtool autoconf libboost-filesystem-dev \
    libboost-iostreams-dev libboost-serialization-dev libboost-thread-dev \
    libboost-test-dev libssl-dev libjsoncpp-dev libcurl4-openssl-dev \
    libjsoncpp-dev libjsonrpccpp-dev libsnappy-dev zlib1g-dev libbz2-dev \
    liblz4-dev libzstd-dev libjemalloc-dev libsparsehash-dev python3-dev \
    python3-pip pkg-config git g++-7 gcc-7 ffmpeg libcairo2 libcairo2-dev

ADD . /blocksci-compilable

# Clone BlockSci repository
# RUN git clone https://github.com/mmstanone/blocksci-compilable.git
RUN CC=gcc-7 CXX=g++ pip install -r /blocksci-compilable/pip-all-requirements.txt

# Build BlockSci
RUN cd blocksci-compilable && \
    rm -r release && \
    mkdir release && \
    cd release && \
    CC=gcc-7 CXX=g++-7 cmake -DCMAKE_BUILD_TYPE=Release .. && \
    make && \
    make install


# Install BlockSci Python bindings
RUN cd blocksci-compilable && \
    CC=gcc-7 CXX=g++-7 pip3 install -e blockscipy

# Set the default command for the container
CMD ["/bin/bash"]

