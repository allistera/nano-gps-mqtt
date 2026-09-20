#!/bin/sh
set -eu

sh scripts/install-arduino-dfu.sh
dfu_path=$(pwd -P)/build/tools/dfu-util/bin

port=${PORT:-$(arduino-cli board list | awk '/usbmodem.*Arduino Nano ESP32/ { print $1; exit }')}
if [ -z "$port" ]; then
  echo "upload: no Arduino Nano ESP32 serial port found; set PORT explicitly" >&2
  exit 1
fi

arduino-cli upload \
  --port "$port" \
  --fqbn esp32:esp32:nano_nora \
  --build-path build/firmware \
  --upload-property "runtime.tools.dfu-util-0.11.0-arduino5.path=$dfu_path" \
  --verify \
  examples/NanoGpsMqtt
