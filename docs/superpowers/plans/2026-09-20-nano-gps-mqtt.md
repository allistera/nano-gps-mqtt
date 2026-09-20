# Nano ESP32 GPS MQTT Prototype Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build and verify Nano ESP32 firmware that reads a GT-U7 GPS receiver continuously and publishes the newest valid fix to an authenticated MQTT broker every 30 seconds.

**Architecture:** Package the reusable firmware as a small Arduino library, with a GPS task and a networking task sharing a mutex-protected latest fix. Keep payload formatting and retry timing independent of Arduino APIs so they can be tested natively with strict compiler warnings before hardware integration.

**Tech Stack:** Arduino CLI 1.5.1, Espressif Arduino ESP32 core 3.3.12, FreeRTOS, TinyGPSPlus 1.0.3, MQTT 2.5.3 by 256dpi, C++17 native tests with Apple Clang.

**Spec:** `docs/superpowers/specs/2026-09-20-nano-gps-mqtt-design.md`

## Global Constraints

- Target board: Arduino Nano ESP32 using FQBN `esp32:esp32:nano_nora`.
- GPS: GT-U7 UART at 9600 baud, Nano RX on `D6`, Nano TX on `D7`.
- Broker: `mqtt.allisterantosik.com:1883`, authenticated, prototype-only plaintext MQTT.
- Publish interval: exactly 30 seconds while connected and a valid fix exists.
- Location topic: `devices/nano-gps-01/location`, QoS 1, not retained.
- Status topic: `devices/nano-gps-01/status`, retained, with retained `offline` last will.
- Credentials remain only in ignored `secrets.h`; never print or commit them.
- Do not add TLS, offline history, OTA, battery management, cellular support, or a backend consumer.

## Review Focus

- Malformed, partial, or checksum-invalid NMEA input must not replace the last valid fix; Task 3 pins this with a parser test sketch.
- NaN, infinity, out-of-range coordinates, and undersized payload buffers must be rejected; Task 2 pins these with native tests.
- `millis()` rollover must not break retry or 30-second scheduling; Task 2 pins unsigned elapsed-time behaviour with native tests.
- Wi-Fi or broker loss must not block GPS parsing or create a tight reconnect loop; Tasks 4 and 5 cover backoff tests and disconnect acceptance.
- A stale retained position must never appear as current; Task 4 asserts non-retained location publishing and Task 5 checks subscription behaviour.

---

### Task 1: Reproducible toolchain and project skeleton

**Files:**
- Create: `.gitignore`
- Create: `library.properties`
- Create: `examples/NanoGpsMqtt/NanoGpsMqtt.ino`
- Create: `examples/NanoGpsMqtt/secrets.example.h`
- Create: `scripts/check-toolchain.sh`
- Create: `scripts/lint.sh`
- Create: `scripts/compile.sh`
- Create: `README.md`

**Interfaces:**
- Consumes: Arduino CLI configuration and the connected Nano ESP32.
- Produces: `scripts/compile.sh` as the canonical firmware build command and the `Secrets` constants consumed by Task 4.

- [ ] **Step 1: Write the toolchain check before installing missing components**

```sh
#!/bin/sh
set -eu

arduino-cli version
arduino-cli core list | grep -F 'esp32:esp32' | grep -F '3.3.12'
arduino-cli lib list | grep -F 'TinyGPSPlus' | grep -F '1.0.3'
arduino-cli lib list | grep -F 'MQTT' | grep -F '2.5.3'
```

- [ ] **Step 2: Run the check and confirm the missing Espressif core or libraries fail it**

Run: `sh scripts/check-toolchain.sh`

Expected: non-zero until all three pinned components are installed.

- [ ] **Step 3: Install the ARM-compatible core and pinned libraries**

```sh
arduino-cli core install esp32:esp32@3.3.12
arduino-cli lib install TinyGPSPlus@1.0.3
arduino-cli lib install MQTT@2.5.3
```

- [ ] **Step 4: Add the library metadata and credential boundary**

`library.properties` must declare `architectures=esp32` and `depends=TinyGPSPlus (1.0.3),MQTT (2.5.3)`. `secrets.example.h` must contain only these empty placeholders:

```cpp
#pragma once

constexpr char WIFI_SSID[] = "";
constexpr char WIFI_PASSWORD[] = "";
constexpr char MQTT_USERNAME[] = "";
constexpr char MQTT_PASSWORD[] = "";
```

`.gitignore` must include:

```gitignore
examples/NanoGpsMqtt/secrets.h
build/
```

Copy `secrets.example.h` to the ignored `secrets.h` so the baseline sketch can
compile without storing working credentials.

- [ ] **Step 5: Add the canonical lint and compile scripts**

`scripts/lint.sh`:

```sh
#!/bin/sh
set -eu

shellcheck scripts/*.sh
```

`scripts/compile.sh`:

```sh
#!/bin/sh
set -eu

arduino-cli compile \
  --fqbn esp32:esp32:nano_nora \
  --warnings all \
  --library . \
  --build-path build/firmware \
  examples/NanoGpsMqtt
```

- [ ] **Step 6: Add a minimal sketch and project instructions**

The sketch should include `secrets.h`, define empty `setup()` and `loop()`, and compile without uploading. The README must document wiring, credential-file creation, the plaintext-port warning, build command, upload command, and serial-monitor command.

- [ ] **Step 7: Verify the toolchain and baseline compile**

Run:

```sh
sh scripts/check-toolchain.sh
sh scripts/compile.sh
git diff --check
```

Expected: all pass; the build targets the Nano ESP32 without the prior x86-only `esptool` failure.

- [ ] **Step 8: Commit the scaffold**

```sh
git add .gitignore README.md library.properties examples scripts
git commit -m "build: scaffold Nano ESP32 firmware"
```

### Task 2: Tested location payload and retry primitives

**Files:**
- Create: `src/LocationFix.h`
- Create: `src/Payload.h`
- Create: `src/Payload.cpp`
- Create: `src/RetryBackoff.h`
- Create: `tests/native/payload_test.cpp`
- Create: `tests/native/retry_backoff_test.cpp`
- Create: `tests/native/main.cpp`
- Create: `scripts/test-native.sh`
- Modify: `scripts/lint.sh`

**Interfaces:**
- Produces: `LocationFix`, `bool formatLocationPayload(const LocationFix&, const char*, char*, size_t)`, and `RetryBackoff::ready(uint32_t)`/`failed(uint32_t)`/`reset()`.
- Consumes: only the C++ standard library, allowing host tests without Arduino headers.

- [ ] **Step 1: Write failing payload tests**

Define a tiny test runner using `assert`. Tests must cover:

```cpp
LocationFix fix{true, 55.953251, -3.188267, 72.4, 0.0, 8, 1.1, 240,
                "2026-09-20T14:32:10Z"};
char payload[256];
assert(formatLocationPayload(fix, "nano-gps-01", payload, sizeof(payload)));
assert(std::strstr(payload, "\"latitude\":55.953251") != nullptr);
assert(std::strstr(payload, "\"satellites\":8") != nullptr);
```

Also assert failure for an invalid fix, latitude outside `[-90, 90]`, longitude outside `[-180, 180]`, NaN/infinity, an empty device ID, and an eight-byte output buffer.

- [ ] **Step 2: Run the native tests and confirm missing interfaces fail compilation**

Run: `sh scripts/test-native.sh`

Expected: compile failure because `LocationFix` and `formatLocationPayload` do not exist.

- [ ] **Step 3: Implement the minimal location model and formatter**

`LocationFix` contains validity, latitude, longitude, altitude metres, speed km/h, satellite count, HDOP, fix age milliseconds, and a 21-character ISO-8601 UTC timestamp plus terminator. The formatter uses bounded `snprintf`, rejects non-finite/out-of-range values, and succeeds only when the complete JSON fits.

- [ ] **Step 4: Run payload tests and confirm they pass**

Run: `sh scripts/test-native.sh`

Expected: all payload assertions pass.

- [ ] **Step 5: Write failing retry and rollover tests**

Test delays of 1, 2, 4, 8, 16, and 30 seconds, the 30-second cap, reset to the initial delay, and unsigned rollover:

```cpp
RetryBackoff retry(1000, 30000);
retry.failed(UINT32_MAX - 500);
assert(!retry.ready(UINT32_MAX - 1));
assert(retry.ready(499));
```

- [ ] **Step 6: Implement `RetryBackoff` and pass the complete native suite**

Use unsigned subtraction for elapsed time. Do not use absolute `now >= deadline` comparisons, which fail at `millis()` rollover.

Extend `scripts/lint.sh` with the strict native compilation command:

```sh
mkdir -p build
clang++ -std=c++17 -Wall -Wextra -Wpedantic -Werror \
  -Isrc tests/native/*.cpp src/Payload.cpp -o build/native-tests
```

Run:

```sh
sh scripts/test-native.sh
sh scripts/lint.sh
```

Expected: both pass with `-Werror`.

- [ ] **Step 7: Commit the tested primitives**

```sh
git add src tests scripts
git commit -m "feat: add tested location payload primitives"
```

### Task 3: GPS ingestion and thread-safe latest-fix store

**Files:**
- Create: `src/LocationStore.h`
- Create: `src/LocationStore.cpp`
- Create: `src/GpsService.h`
- Create: `src/GpsService.cpp`
- Create: `extras/tests/GpsParserTest/GpsParserTest.ino`
- Modify: `scripts/compile.sh`

**Interfaces:**
- Consumes: TinyGPSPlus, `LocationFix`, and a hardware `Stream` containing NMEA bytes.
- Produces: `GpsService::poll(Stream&, uint32_t nowMs)` and `LocationStore::read(LocationFix&)`/`write(const LocationFix&)`.

- [ ] **Step 1: Add a parser test sketch with known NMEA fixtures**

Feed a valid GGA/RMC pair one character at a time and assert the expected latitude, longitude, satellite count, UTC timestamp, and valid flag. Then feed a partial sentence and a sentence with a bad checksum and assert the stored fix does not change.

- [ ] **Step 2: Compile the test and confirm the missing GPS interfaces fail**

Run:

```sh
arduino-cli compile --fqbn esp32:esp32:nano_nora --warnings all \
  --library . extras/tests/GpsParserTest
```

Expected: compile failure because `GpsService` and `LocationStore` do not exist.

- [ ] **Step 3: Implement the mutex-protected store**

Create the mutex in the constructor, take it with a bounded wait for reads and writes, copy the complete `LocationFix` while held, and release it on every path. Return `false` if the mutex cannot be acquired.

- [ ] **Step 4: Implement incremental GPS parsing**

`GpsService::poll` drains only currently available bytes, updates the store only after TinyGPSPlus reports a newly updated valid location and valid UTC date/time, and computes fix age from TinyGPSPlus. It must never wait for a complete sentence.

- [ ] **Step 5: Compile and run the parser test on the Nano**

Run:

```sh
arduino-cli compile --fqbn esp32:esp32:nano_nora --warnings all \
  --library . extras/tests/GpsParserTest
arduino-cli upload -p /dev/cu.usbmodem1101 \
  --fqbn esp32:esp32:nano_nora extras/tests/GpsParserTest
arduino-cli monitor -p /dev/cu.usbmodem1101 --config baudrate=115200
```

Expected: serial output reports all parser assertions passed, including partial and invalid-checksum cases.

- [ ] **Step 6: Run all automated checks**

Run:

```sh
sh scripts/test-native.sh
sh scripts/lint.sh
sh scripts/compile.sh
git diff --check
```

Expected: all pass.

- [ ] **Step 7: Commit GPS ingestion**

```sh
git add src extras scripts
git commit -m "feat: add continuous GPS ingestion"
```

### Task 4: Wi-Fi and MQTT service

**Files:**
- Create: `src/NetworkConfig.h`
- Create: `src/NetworkService.h`
- Create: `src/NetworkService.cpp`
- Create: `tests/native/publish_schedule_test.cpp`
- Modify: `tests/native/main.cpp`
- Modify: `scripts/test-native.sh`

**Interfaces:**
- Consumes: `NetworkConfig`, `LocationStore`, `formatLocationPayload`, Wi-Fi credentials, MQTT credentials, and `RetryBackoff`.
- Produces: `NetworkService::begin()` and non-blocking `NetworkService::poll(uint32_t nowMs)` for use by a dedicated FreeRTOS task.

- [ ] **Step 1: Add failing native schedule tests**

Extract `PublishSchedule` as a standard-C++ helper. Test that it is initially ready, becomes unready immediately after `published(now)`, becomes ready at 30,000 ms, and remains correct across `UINT32_MAX` rollover.

- [ ] **Step 2: Implement the schedule helper and pass native tests**

Use unsigned elapsed-time subtraction, as with retry timing.

Run: `sh scripts/test-native.sh`

Expected: all tests pass.

- [ ] **Step 3: Define explicit network configuration**

```cpp
struct NetworkConfig {
  const char *wifiSsid;
  const char *wifiPassword;
  const char *mqttHost;
  uint16_t mqttPort;
  const char *mqttUsername;
  const char *mqttPassword;
  const char *deviceId;
  const char *locationTopic;
  const char *statusTopic;
  uint32_t publishIntervalMs;
};
```

- [ ] **Step 4: Implement non-blocking connection management**

`poll()` must call `MQTTClient::loop()` while connected, attempt Wi-Fi and MQTT reconnection only when their backoffs are ready, call `setWill(statusTopic, "offline", true, 1)` before `connect`, and publish retained `online` at QoS 1 after a successful MQTT connection. Serial messages may contain state and numeric error codes but no secret fields.

- [ ] **Step 5: Implement publishing semantics**

At each due interval, read one snapshot from `LocationStore`; skip invalid fixes; format into a fixed 384-byte buffer; call `MQTTClient::publish(locationTopic, payload, false, 1)`; advance the schedule only after a successful publish. Never block waiting for a fix.

- [ ] **Step 6: Compile with maximum project warnings and run lint**

Run:

```sh
sh scripts/test-native.sh
sh scripts/lint.sh
sh scripts/compile.sh
git diff --check
```

Expected: all pass.

- [ ] **Step 7: Commit networking**

```sh
git add src tests scripts
git commit -m "feat: add resilient MQTT publishing"
```

### Task 5: FreeRTOS integration and live acceptance

**Files:**
- Modify: `examples/NanoGpsMqtt/NanoGpsMqtt.ino`
- Modify: `README.md`
- Create locally, never add: `examples/NanoGpsMqtt/secrets.h`

**Interfaces:**
- Consumes: `GpsService`, `LocationStore`, `NetworkService`, hardware `Serial1`, and local secrets.
- Produces: complete flashable firmware and documented operator workflow.

- [ ] **Step 1: Integrate the two tasks**

Initialize `Serial1` as `Serial1.begin(9600, SERIAL_8N1, D6, D7)`. Create a GPS task that repeatedly calls `GpsService::poll` and delays one tick, and a network task that calls `NetworkService::poll(millis())` and delays ten ticks. Check every `xTaskCreate` return value and stop startup with a safe diagnostic if task creation fails.

- [ ] **Step 2: Create the ignored local secrets file without echoing values**

Copy `secrets.example.h` to `secrets.h`, then populate it through a local editor or protected credential retrieval. Verify only that all four strings are non-empty; do not print their values.

- [ ] **Step 3: Run the complete local quality gate**

Run:

```sh
sh scripts/check-toolchain.sh
sh scripts/test-native.sh
sh scripts/lint.sh
sh scripts/compile.sh
git diff --check
```

Expected: all checks pass.

- [ ] **Step 4: Verify secrets are not staged or present in history**

Run:

```sh
git status --short
git check-ignore examples/NanoGpsMqtt/secrets.h
git grep -n -E 'WIFI_PASSWORD|MQTT_PASSWORD' -- ':!*.example.h'
```

Expected: `secrets.h` is ignored; grep finds variable references only, never credential values.

- [ ] **Step 5: Flash and observe the connected board**

Run:

```sh
arduino-cli upload -p /dev/cu.usbmodem1101 \
  --fqbn esp32:esp32:nano_nora examples/NanoGpsMqtt
arduino-cli monitor -p /dev/cu.usbmodem1101 --config baudrate=115200
```

Expected: safe diagnostics show GPS input, valid fix acquisition outdoors, Wi-Fi connection, MQTT connection, and successful publishes without exposing credentials.

- [ ] **Step 6: Independently verify broker output**

Using an authenticated MQTT subscriber whose password is supplied without shell-history exposure, subscribe to `devices/nano-gps-01/#`. Confirm status becomes retained `online`, location JSON arrives every 30 seconds, coordinate fields are plausible, timestamps advance, and location messages are not retained for a new subscriber.

- [ ] **Step 7: Exercise failure recovery**

Interrupt Wi-Fi for at least 45 seconds. Confirm the broker records the retained `offline` last will, the GPS task continues processing fixes, retries remain bounded, restored Wi-Fi leads to retained `online`, and only the newest fix is published after recovery.

- [ ] **Step 8: Finalize operating documentation and commit**

Record the exact verified build/upload/monitor commands, expected topics, payload example, wiring, outdoor-fix requirement, and the mandatory TLS production follow-up in README.

```sh
git add examples/NanoGpsMqtt/NanoGpsMqtt.ino README.md
git commit -m "feat: integrate Nano GPS MQTT firmware"
```

- [ ] **Step 9: Review final state before delivery**

Run:

```sh
git status --short
git log --oneline --decorate -5
git diff HEAD~3..HEAD --check
```

Expected: clean working tree, focused commits, and no whitespace errors. If no remote has been configured, report that push remains unavailable rather than claiming publication.
