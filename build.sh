#!/bin/bash

mkdir -p build/Debug
mkdir -p build/Release

pushd build/Debug
cmake -DCMAKE_BUILD_TYPE=DEBUG -DCMAKE_EXPORT_COMPILE_COMMANDS=1 ../../
ln -f -s build/Debug/compile_commands.json ../../
popd

pushd build/Release
cmake -DCMAKE_BUILD_TYPE=RELEASE ../../
popd

cmake --build build/Debug
cmake --build build/Release
