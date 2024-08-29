#!/bin/bash

cd build
CC=gcc-7 CXX=g++-7 cmake -DCMAKE_BUILD_TYPE=Release ..
make -j 258
make install

cd ..
CC=gcc-7 CXX=g++-7 uv run pip3 install -e blockscipy

cd Notebooks
uv run jupyter notebook --ip=0.0.0.0 --allow-root
