#!/bin/sh
set -eu

mkdir -p build
clang++ -std=c++17 -Wall -Wextra -Wpedantic -Werror \
  -Isrc tests/native/*.cpp src/Payload.cpp -o build/native-tests
build/native-tests
