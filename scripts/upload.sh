#!/bin/sh
set -eu

sh scripts/install-arduino-dfu.sh
dfu_path=$(pwd -P)/build/tools/dfu-util/bin

arduino-cli upload \
  --port /dev/cu.usbmodem1101 \
  --fqbn esp32:esp32:nano_nora \
  --build-path build/firmware \
  --upload-property "runtime.tools.dfu-util-0.11.0-arduino5.path=$dfu_path" \
  --verify \
  examples/NanoGpsMqtt
