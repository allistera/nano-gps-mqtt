#include <Arduino.h>

#include <GpsService.h>
#include <LocationFix.h>
#include <LocationStore.h>

#include <cmath>
#include <cstring>

class BufferStream : public Stream {
 public:
  explicit BufferStream(const char *input) : input_(input) {}

  int available() override {
    return static_cast<int>(std::strlen(input_ + position_));
  }

  int read() override {
    if (input_[position_] == '\0') {
      return -1;
    }
    return input_[position_++];
  }

  int peek() override {
    if (input_[position_] == '\0') {
      return -1;
    }
    return input_[position_];
  }

  size_t write(uint8_t) override { return 0; }

 private:
  const char *input_;
  size_t position_ = 0;
};

LocationStore locationStore;
GpsService gpsService(locationStore);
bool testsPassed = true;

void check(bool condition, const char *message) {
  if (!condition) {
    testsPassed = false;
    Serial.print("GPS parser test failed: ");
    Serial.println(message);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  BufferStream validNmea(
      "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47\r\n"
      "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230324,003.1,W*61\r\n");
  gpsService.poll(validNmea, 1000);

  LocationFix expected{};
  check(locationStore.read(expected), "valid fix was not stored");
  check(expected.valid, "stored fix was not marked valid");
  check(std::fabs(expected.latitude - 48.1173) < 0.000001,
        "latitude did not match fixture");
  check(std::fabs(expected.longitude - 11.516667) < 0.000001,
        "longitude did not match fixture");
  check(expected.satellites == 8, "satellite count did not match fixture");
  check(std::strcmp(expected.timestamp, "2024-03-23T12:35:19Z") == 0,
        "timestamp did not match fixture");

  BufferStream partialNmea("$GPRMC,130000,A,5100.000,N");
  gpsService.poll(partialNmea, 2000);
  LocationFix afterPartial{};
  check(locationStore.read(afterPartial), "fix was lost after partial sentence");
  check(afterPartial.latitude == expected.latitude,
        "partial sentence changed latitude");
  check(afterPartial.longitude == expected.longitude,
        "partial sentence changed longitude");

  BufferStream invalidChecksum(
      "$GPRMC,130000,A,5100.000,N,00100.000,W,0.0,0.0,230394,,,A*00\r\n");
  gpsService.poll(invalidChecksum, 3000);
  LocationFix afterInvalid{};
  check(locationStore.read(afterInvalid),
        "fix was lost after invalid checksum");
  check(afterInvalid.latitude == expected.latitude,
        "invalid checksum changed latitude");
  check(afterInvalid.longitude == expected.longitude,
        "invalid checksum changed longitude");

  Serial.println(testsPassed ? "GPS parser tests passed"
                             : "GPS parser tests failed");
}

void loop() {
  Serial.println(testsPassed ? "GPS parser tests passed"
                             : "GPS parser tests failed");
  delay(1000);
}
