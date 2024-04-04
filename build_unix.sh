#!/bin/bash
mkdir -p build/CMake/Debug
mkdir -p build/CMake/Release
mkdir -p bin/Debug
mkdir -p bin/Release
cd build/CMake/Release
cmake -S ../../ -B .
make && make Shaders
cd ../../bin/Release
./sumire
cd .././