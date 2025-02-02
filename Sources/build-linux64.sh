#!/bin/bash

NCPU=`cat /proc/cpuinfo |grep vendor_id |wc -l`
let NCPU=$NCPU+2
echo "Will build with 'make -j$NCPU' ... please edit this script if incorrect."

# Report all progress and terminate it if some command returns an error code
set -e
set -x

# Delete Bison/Flex generated files
rm -f ./Ecc/Parser.cpp
rm -f ./Ecc/Parser.h
rm -f ./Ecc/Parser.hpp
rm -f ./Ecc/Scanner.cpp
rm -f ./Engine/Base/Parser.cpp
rm -f ./Engine/Base/Parser.h
rm -f ./Engine/Base/Parser.hpp
rm -f ./Engine/Base/Scanner.cpp
rm -f ./Engine/Ska/smcPars.cpp
rm -f ./Engine/Ska/smcPars.h
rm -f ./Engine/Ska/smcPars.hpp
rm -f ./Engine/Ska/smcScan.cpp

# NOTE: To rebuild everything from scratch, add "rebuild" argument to the script

# If rebuilding everything or there's no build directory yet
if [[ "$1" == "rebuild" ]] || ! [[ -d cmake-build ]]; then
  # Delete old CMake build directory and create a new one
  rm -rf cmake-build
  mkdir $_
  cd $_
else
  # Enter existing build directory
  cd cmake-build
fi

#cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -DSE1_BUILD_X86=1 ..
#ninja

# Available CMAKE_BUILD_TYPE types:
# - Debug
# - Release
# - MinSizeRel
# - RelWithDebInfo
cmake -DCMAKE_BUILD_TYPE=Release -DSDL_SHARED=0 -DSDL_STATIC=1 -DSE1_COMPILE_ES=1 ..

echo "ECC first"
make ecc
echo "Then the rest..."
make -j$NCPU
