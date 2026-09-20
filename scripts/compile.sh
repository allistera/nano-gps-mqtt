#!/bin/sh
set -eu

sh scripts/install-arduino-ctags.sh
ctags_path=$(pwd -P)/build/tools/ctags/bin

arduino-cli compile \
  --fqbn esp32:esp32:nano_nora \
  --warnings all \
  --library . \
  --build-property "runtime.tools.ctags.path=$ctags_path" \
  --build-path build/firmware \
  examples/NanoGpsMqtt
