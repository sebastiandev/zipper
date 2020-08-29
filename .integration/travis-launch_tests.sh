#!/bin/bash

# Compile
mkdir -p build
cd build || exit 1
cmake .. || exit 1
make -j4 || exit 1
cd ..

# Unit test with valgrind to track possible memory leaks
(cd build && valgrind --track-origins=yes ./Zipper-test) || exit 1
