#!/bin/sh
set -eu

arduino-cli version
arduino-cli core list | grep -F 'esp32:esp32' | grep -F '3.3.12'
arduino-cli lib list | grep -F 'TinyGPSPlus' | grep -F '1.0.3'
arduino-cli lib list | grep -F 'MQTT' | grep -F '2.5.3'
autoreconf --version >/dev/null
pkg-config --exists libusb-1.0
