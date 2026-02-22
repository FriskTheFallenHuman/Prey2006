#!/bin/sh

BUILD_DIR=build/ninja
BUILDTYPE="debug"
CMAKE_BUILD_TYPE="Debug"
CMAKE_GENERATOR="Ninja"

if [ "$#" -ne "2" ]; then
  echo "Usage cmake_linux.sh <gcc|clang|clang-libc++> <debug|release|reldeb>"
  exit 1
fi  

if [ "$#" -ge "1" ]; then 

  if [ "$1" = "gcc" ]; then
    export CXX="g++"
    export CC="gcc"
  fi

  if [ "$1" = "clang" ]; then
    export CXX="clang++"
    export CC="clang"   
    CMAKE_CXX_FLAGS="-stdlib=libstdc++"
  fi 

  if [ "$1" = "clang-libc++" ]; then
    export CXX="clang++"
    export CC="clang"   
    CMAKE_CXX_FLAGS="-stdlib=libc++"
  fi  
fi

if [ "$#" -ge "2" ]; then 

  if [ "$2" = "debug" ]; then
    BUILDTYPE="debug"
    CMAKE_BUILD_TYPE="Debug"
  fi

  if [ "$2" = "release" ]; then
    BUILDTYPE="release"
    CMAKE_BUILD_TYPE="Release"
  fi  

  if [ "$2" = "reldeb" ]; then
    BUILDTYPE="relwithdebinfo"
    CMAKE_BUILD_TYPE="relwithdebinfo"
  fi    
fi

mkdir -p $BUILD_DIR
cd $BUILD_DIR

cmake -G"$CMAKE_GENERATOR" -DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE -DCMAKE_CXX_FLAGS="$CMAKE_CXX_FLAGS" -DFORCE_COLORED_OUTPUT=ON ../../neo