#!/bin/bash

### This script will git clone some libraries that Zipper needs and
### compile them. To avoid pollution, they are not installed into your
### environement. Therefore Zipper Makefiles have to know where to
### find their files (includes and static/shared libraries).

### $1 is given by ../Makefile and refers to the current architecture.
if [ "$1" == "" ]; then
  echo "Expected one argument. Select the architecture: Linux, Darwin or Windows"
  exit 1
fi
ARCHI="$1"
TARGET=Zipper

function print-compile
{
    echo -e "\033[35m*** Compiling:\033[00m \033[36m$TARGET\033[00m <= \033[33m$1\033[00m"
}

### Library zlib-ng
print-compile zlib-ng
if [ -e zlib-ng ];
then
    mkdir -p zlib-ng/build
    (cd zlib-ng/build
     cmake -DZLIB_COMPAT=ON -DZLIB_ENABLE_TESTS=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON ..
     VERBOSE=1 make -j`nproc --all`
    )
else
    echo "Failed compiling external/zipper: directory does not exist"
fi

### Library minizip
print-compile minizip
if [ -e minizip ];
then
    mkdir -p minizip/build
    (cd minizip/build
     cmake ..
     VERBOSE=1 make -j`nproc --all`
    )
else
    echo "Failed compiling external/zipper: directory does not exist"
fi
