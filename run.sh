#!/bin/bash

mkdir -p build
cd build

cmake ..
cmake --build .
ctest --output-on-failure