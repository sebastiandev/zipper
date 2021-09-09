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

### Delete all previous directories to be sure to have and compile
### fresh code source.
rm -fr backward-cpp zlib-ng minizip 2> /dev/null

function clone
{
    NAME=`echo "$1" | cut -d"/" -f2`
    echo -e "\033[35m*** Cloning:\033[00m \033[36m$TARGET\033[00m <= \033[33m$NAME\033[00m"
    git clone --recursive https://github.com/$1.git --depth=1 #> /dev/null 2> /dev/null
}

### zlib replacement with optimizations for "next generation" systems.
### License: zlib
clone zlib-ng/zlib-ng

### Minizip contrib in zlib with latest bug fixes that supports PKWARE disk spanning, AES encryption, and IO buffering
### License: zlib
clone Lecrapouille/minizip
