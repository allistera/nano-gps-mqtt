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

Any file named `secrets.h` is ignored repository-wide, and lint/CI rejects one
if it is force-added. Never commit or print it. The prototype uses authenticated,
unencrypted MQTT on port 1883. Move the broker and firmware to TLS on port 8883
before using the device in production.

## Build and upload

The first build and upload compile Arduino's pinned `ctags` and patched
`dfu-util` tools locally because Arduino does not publish macOS ARM binaries for
those required versions. This avoids a system-wide Rosetta dependency. Building
the tools requires Homebrew `autoconf`, `automake`, `pkgconf`, and `libusb`.

```sh
sh scripts/check-toolchain.sh
sh scripts/test-native.sh
sh scripts/lint.sh
sh scripts/compile.sh
sh scripts/upload.sh
arduino-cli monitor -p "$(arduino-cli board list | awk '/usbmodem/ { print $1; exit }')" \
  --config baudrate=115200
```

`scripts/upload.sh` detects the Nano ESP32 serial port automatically. Set
`PORT=/dev/cu.usbmodemXXXX` to override it. Startup diagnostics print within a
second of reset, so open the monitor before pressing reset to see them.

The on-device GPS parser test lives in `extras/tests/GpsParserTest` and can be
compiled and uploaded with the same `arduino-cli` flags by pointing at that
sketch directory instead of `examples/NanoGpsMqtt`.

## Runtime behaviour

The firmware runs two FreeRTOS tasks. The GPS task drains `Serial1` (9600 baud
on `D6`/`D7`) continuously and stores the newest valid fix. The network task
maintains Wi-Fi and MQTT with independent bounded exponential backoff (1 s to
30 s) and publishes every 30 seconds while connected. Serial diagnostics at
115200 baud report startup, connection state changes with numeric error codes,
GPS byte counts, and publish results. They never include credentials.

The GT-U7 needs a clear view of the sky for a first fix, which can take several
minutes from cold. Indoors the diagnostics report GPS bytes arriving but no
valid fix, and nothing is published until a fix exists.

## MQTT contract

Broker: `mqtt.allisterantosik.com:1883`, authenticated. The client ID is the
device ID suffixed with the ESP32 factory MAC address.

| Topic | QoS | Retained | Payload |
| --- | --- | --- | --- |
| `devices/nano-gps-01/status` | 1 | yes | `online` after connecting; `offline` last will |
| `devices/nano-gps-01/location` | 1 | no | JSON fix, see below |

Location messages are deliberately not retained so a stale position is never
presented as current to a new subscriber.

```json
{
  "device_id": "nano-gps-01",
  "timestamp": "2026-09-20T14:32:10Z",
  "latitude": 55.953251,
  "longitude": -3.188267,
  "altitude_m": 72.4,
  "speed_kph": 0.0,
  "satellites": 8,
  "hdop": 1.1,
  "fix_age_ms": 240
}
```

To verify from another machine, subscribe to `devices/nano-gps-01/#` with an
authenticated MQTT client, supplying the password through a prompt or
environment variable rather than on the command line.

## Production follow-up

Before any use beyond the prototype, move the broker and firmware to TLS on
port 8883. Plaintext MQTT exposes the credentials on the network.
