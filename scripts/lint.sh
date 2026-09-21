#!/bin/sh
set -eu

shellcheck scripts/*.sh tests/*.sh
sh scripts/check-no-secrets.sh

mkdir -p build
clang++ -std=c++17 -Wall -Wextra -Wpedantic -Werror \
  -Isrc tests/native/*.cpp src/Payload.cpp -o build/native-tests
