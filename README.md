# Nano ESP32 GPS MQTT

Arduino Nano ESP32 prototype firmware that reads a GT-U7 GPS receiver and
publishes the newest valid location to MQTT every 30 seconds.

## Wiring

For a USB-powered prototype, connect the modules as follows:

| GT-U7 | Nano ESP32 | Purpose |
| --- | --- | --- |
| `VCC` | `VBUS` | 5 V USB power |
| `GND` | `GND` | Common ground |
| `TXD` | `D6` | GPS data into the Nano |
| `RXD` | `D7` | Optional commands to the GPS |
| `PPS` | Unconnected | Not used |

Confirm that the particular GT-U7 board accepts 5 V power and produces 3.3 V
UART logic before applying power.

## Credentials

Create the ignored local credentials file and populate it locally:

```sh
cp examples/NanoGpsMqtt/secrets.example.h examples/NanoGpsMqtt/secrets.h
```

Never commit or print `secrets.h`. The prototype uses authenticated, unencrypted
MQTT on port 1883. Move the broker and firmware to TLS on port 8883 before using
the device in production.

## Build and upload

The first build compiles Arduino's pinned `ctags` tool locally because Arduino
does not publish a macOS ARM binary for that required version. This avoids a
system-wide Rosetta dependency.

```sh
sh scripts/check-toolchain.sh
sh scripts/lint.sh
sh scripts/compile.sh
arduino-cli upload -p /dev/cu.usbmodem1101 \
  --fqbn esp32:esp32:nano_nora examples/NanoGpsMqtt
arduino-cli monitor -p /dev/cu.usbmodem1101 --config baudrate=115200
```
