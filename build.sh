#!/bin/bash

mkdir -p build/Debug
mkdir -p build/Release

pushd build/Debug
cmake -DCMAKE_BUILD_TYPE=DEBUG ../../
popd

pushd build/Release
cmake -DCMAKE_BUILD_TYPE=RELEASE ../../
popd

cmake --build build/Debug
cmake --build build/Release
