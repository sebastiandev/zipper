#!/bin/bash

export PATH="/mingw64/bin"
cd ${APPVEYOR_BUILD_FOLDER}

# Compile
cmake -B build -GNinja .
cd build || exit 1
cmake --build . || exit 1
cd ..

# Unit test
(cd build && ./Zipper-test) || exit 1
