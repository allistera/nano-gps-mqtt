# Nano ESP32 GPS MQTT Prototype Design

## Goal

Build a USB-powered Arduino Nano ESP32 prototype that continuously reads a
GT-U7 GPS receiver and publishes its latest valid position to an existing MQTT
broker every 30 seconds over Wi-Fi.

The prototype broker endpoint is `mqtt.allisterantosik.com:1883`. Port 1883
does not encrypt traffic, so this endpoint is suitable only for prototype use.
Moving to TLS on port 8883 is required before production deployment.

## Architecture

Use the Arduino ESP32 core and its built-in FreeRTOS scheduler. Two application
tasks keep GPS ingestion independent from potentially slow network operations:

1. The GPS task continuously reads and parses NMEA data from a hardware UART.
2. The networking task maintains Wi-Fi and MQTT connections and publishes the
   newest valid GPS fix every 30 seconds.

The tasks share a small latest-fix value protected by a FreeRTOS mutex. The
device does not keep an unbounded history or replay stale fixes after an outage.

## Hardware

Connect the GT-U7 to the Nano ESP32 as follows for a USB-powered prototype:

| GT-U7 | Nano ESP32 | Purpose |
| --- | --- | --- |
| `VCC` | `VBUS` | 5 V USB power |
| `GND` | `GND` | Common ground |
| `TXD` | `D6` | GPS UART data into the Nano |
| `RXD` | `D7` | Optional commands from the Nano |
| `PPS` | Unconnected | Not required by this prototype |

The UART uses 9600 baud. The particular GT-U7 board markings and power input
must be visually confirmed before power is applied because third-party GT-U7
boards vary. Its UART TX signal must be 3.3 V logic for direct connection to the
Nano ESP32.

## MQTT Contract

The authenticated MQTT connection uses a unique client ID derived from the
ESP32 hardware identity.

- Location topic: `devices/nano-gps-01/location`
- Availability topic: `devices/nano-gps-01/status`
- Quality of service: QoS 1
- Location retention: disabled, to avoid presenting an old position as current
- Availability retention: enabled
- Last will: retained `offline` on the availability topic

The device publishes `online` after connecting. A location message has this
shape:

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

The firmware publishes only valid fixes. It uses GPS UTC for `timestamp` and
records the age of the shared fix when constructing the message.

## Connectivity and failure handling

Wi-Fi and MQTT reconnect independently with bounded exponential backoff. GPS
reading continues during reconnect attempts. After connectivity returns, the
next scheduled message contains only the newest valid fix.

Serial diagnostics may report startup, connection state, GPS fix state, and
publish results. They must never include Wi-Fi or MQTT credentials.

## Credentials

Wi-Fi SSID/password and MQTT username/password live in a local `secrets.h`
excluded from version control. A committed `secrets.example.h` documents the
required fields without containing working credentials. Credentials must not be
printed, committed, or included in build logs.

## Verification

Verification proceeds in observable stages:

1. Confirm raw NMEA input from the GT-U7 at 9600 baud.
2. Obtain an outdoor GPS fix and confirm parsed coordinates.
3. Compile the firmware with warnings enabled.
4. Upload it to the detected Arduino Nano ESP32 and inspect safe diagnostics.
5. Subscribe independently to `devices/nano-gps-01/#` and confirm valid JSON
   location messages arrive every 30 seconds.
6. Interrupt and restore Wi-Fi, then confirm automatic recovery and correct
   retained availability transitions.
7. Confirm no credential appears in source control, serial output, or logs.

## Out of scope

TLS, offline location history, over-the-air firmware updates, battery/low-power
operation, cellular connectivity, and a backend consumer are outside this
prototype. TLS on port 8883 is a required follow-up before production use.
