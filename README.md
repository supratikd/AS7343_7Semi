# AS7343 7Semi Arduino Library

Arduino library for the 7Semi AS7343 spectral sensor board. It provides a
small high-level API for spectral measurements, the board's illumination LED,
VIS/CLEAR readings, and the AS7343 flicker-detection block.

The spectral getters return **raw sensor counts**. They are not calibrated
irradiance, lux, or color values.

## Why this library?

Working examples for the 7Semi AS7343 board are hard to find, and the options
available when this project was started were limited:

- **The official 7Semi library failed to compile.** Building its `Basic_read`
  example produced:

  ```text
  7Semi_AS7343.h:1250:10: error: extra qualification 'AS7343_7Semi::'
  on member 'getRGB' [-fpermissive]
   1250 |     bool AS7343_7Semi::getRGB(uint16_t &r, uint16_t &g, uint16_t &b);
  ```

  This is the library at
  [7semi-solutions/7Semi-AS7343](https://github.com/7semi-solutions/7Semi-AS7343)
  at the time this project was started; the issue may have been fixed since,
  so the repository's current state should be checked.
- **It was not available in the Arduino Library Manager**, so it could not be
  installed with a single click.
- **Few third-party repositories were available, and their trustworthiness
  was difficult to assess.** Source code should be reviewed before an
  unfamiliar library is added to an Arduino sketchbook.

This library aims to be a small, readable, working alternative:

- It compiles cleanly and ships with tested examples (ESP32 and Arduino Uno
  builds were verified; behavior on hardware depends on your board).
- The API is short and descriptive (`blue_FZ()`, `orange_FXL()`,
  `getVIS()`, ...), with the channel-to-wavelength mapping documented below.
- The code is small enough to audit in a few minutes: an I2C helper
  (`src/core`), the sensor driver (`src/sensor`), and a thin public wrapper.
- It covers what most projects need: raw spectral channels, VIS, LED control,
  built-in flicker classification, and FIFO-based frequency estimation.

## Features

- Read the AS7343's 12 named spectral channels through descriptive getters.
- Read the VIS/CLEAR value averaged from the six VIS slots in 18-channel
  Auto-SMUX mode.
- Choose a spectral gain and configure the library's fixed integration preset.
- Control the board's illumination LED.
- Classify supported 100 Hz / 120 Hz flicker and estimate other frequencies
  from flicker FIFO samples.
- Start the sensor on the default I2C bus or pass a `TwoWire` bus and, on
  ESP32/ESP8266, custom SDA/SCL pins.

## Requirements

- Arduino IDE or Arduino CLI
- A supported Arduino board with `Wire`/I2C support
- 7Semi AS7343 sensor board connected to the board's I2C bus

This library uses I2C address `0x59`. Check your board documentation and wiring
if the sensor does not respond at that address. Ensure the board and host use
compatible logic levels and a common ground.

## Installation

### Arduino IDE

1. Download or clone this repository.
2. Install the `AS7343_7Semi` folder in your Arduino libraries directory, or
   use **Sketch > Include Library > Add .ZIP Library...** with a ZIP of this
   repository.
3. Restart the IDE if the library does not appear.
4. Open an example from **File > Examples > AS7343_7Semi**.

### Arduino CLI

Place the downloaded or cloned `AS7343_7Semi` repository in the Arduino CLI
sketchbook's `libraries` directory. Then compile an example for your board,
for example:

```sh
arduino-cli compile --fqbn esp32:esp32:esp32 examples/00_GetRAWValues
```

The Spectral Monitor example uses ESP32's `WiFi` and `WebServer` libraries and
is intended for ESP32.

## Quick start: spectral readings

```cpp
#include <AS7343_7Semi.h>

AS7343_7Semi sensor;

void setup() {
  Serial.begin(115200);

  if (!sensor.begin()) {
    Serial.println("AS7343 initialization failed");
    while (true) {
      delay(1000);
    }
  }

  sensor.powerOn();

  // Select the 18-slot Auto-SMUX layout used by this library's channel map.
  sensor.configureAutoSMUX();

  // Optional: use a known integration time and gain for repeatable readings.
  sensor.setIntegration();
  sensor.setGain(AS7343_7Semi::Gain::X32);
}

void loop() {
  if (!sensor.sensorPresent()) {
    Serial.println("AS7343 not responding");
    delay(1000);
    return;
  }

  if (sensor.read()) {
    Serial.print("Violet F1, 405 nm: ");
    Serial.println(sensor.violet_F1());

    Serial.print("Blue FZ, 450 nm: ");
    Serial.println(sensor.blue_FZ());

    Serial.print("Near-IR NIR, 855 nm: ");
    Serial.println(sensor.nearInfrared_NIR());
  } else {
    Serial.println("Spectral measurement failed");
  }

  delay(200);
}
```

`begin()` starts the selected I2C bus and checks that the sensor's
identification registers respond. It does not select the desired spectral
gain, integration time, Auto-SMUX layout, or illumination LED state; configure
those for your application after a successful `begin()`.

`read()` starts a spectral conversion, waits for valid data (up to one second),
reads the 18-slot dataset, and updates the channel getters. Only use getter
values after `read()` succeeds; they otherwise retain the previous successful
sample (or their initial zero values).

## Spectral channel getters

Call `read()` successfully before retrieving channel values. Each getter
returns the raw count from the corresponding Auto-SMUX slot.

| Getter | Channel | Wavelength | Region |
|---|---:|---:|---|
| `violet_F1()` | F1 | 405 nm | Violet |
| `violetBlue_F2()` | F2 | 425 nm | Violet-Blue |
| `blue_FZ()` | FZ | 450 nm | Blue |
| `blueCyan_F3()` | F3 | 475 nm | Blue-Cyan |
| `green_F4()` | F4 | 515 nm | Green |
| `greenYellow_F5()` | F5 | 550 nm | Green-Yellow |
| `yellowGreen_FY()` | FY | 555 nm | Yellow-Green |
| `orange_FXL()` | FXL | 600 nm | Orange |
| `red_F6()` | F6 | 640 nm | Red |
| `deepRed_F7()` | F7 | 690 nm | Deep Red |
| `redNearInfrared_F8()` | F8 | 745 nm | Red-Near-IR |
| `nearInfrared_NIR()` | NIR | 855 nm | Near-IR |
| `getVIS()` | VIS/CLEAR average | -- | Visible broadband |

`getVIS()` is the integer average of the six VIS values collected across the
three Auto-SMUX cycles. It is a broadband channel and does not have one
specific wavelength.

## Sensor configuration

### Auto-SMUX

```cpp
sensor.configureAutoSMUX();
```

Selects the sensor's 18-channel automatic SMUX mode. This is needed when using
the library's complete spectral mapping and `getVIS()` calculation. Skip it
only if you have intentionally configured a different sensor mode yourself;
in that case, the library's channel-to-slot mapping may not describe the data
you receive.

### Integration time

```cpp
sensor.setIntegration();
```

Applies the integration preset currently built into this library: ATIME=29 and
ASTEP=599. Use it when consistent settings across starts are needed or when
using the example configuration. It is not required if the integration
registers have already been configured and those settings should be retained.

### Gain

```cpp
sensor.setGain(AS7343_7Semi::Gain::X32);
```

Sets the spectral analog gain. Choose it for the light level and integration
time: lower gain can help prevent saturation in bright conditions; higher gain
can improve response in dim conditions. A fixed gain is useful for comparing
measurements, but is not mandatory if another part of your application manages
gain.

`configureAutoSMUX()`, `setIntegration()`, and `setGain()` print diagnostic
messages to `Serial` when called.

Available values:

| Enum | Gain |
|---|---:|
| `Gain::X0_5` | 0.5x |
| `Gain::X1` | 1x |
| `Gain::X2` | 2x |
| `Gain::X4` | 4x |
| `Gain::X8` | 8x |
| `Gain::X16` | 16x |
| `Gain::X32` | 32x |
| `Gain::X64` | 64x |
| `Gain::X128` | 128x |
| `Gain::X256` | 256x |
| `Gain::X512` | 512x |
| `Gain::X1024` | 1024x |
| `Gain::X2048` | 2048x |

### Power and illumination LED

```cpp
sensor.powerOn();
sensor.ledOn();
sensor.ledOff();
```

`powerOn()` enables the sensor. `ledOn()` and `ledOff()` control the board's
illumination LED; they do not control whether spectral conversions run. The
library does not automatically turn the LED off after a read. Use ambient
light instead of the board LED by calling `ledOff()` if that is what your
measurement requires.

## Flicker detection

The AS7343's `FD_STATUS` register classifies supported 100 Hz and 120 Hz
flicker; it does not return an arbitrary frequency value. To estimate other
frequencies, the sensor can buffer raw flicker samples in its FIFO and the
library can estimate frequency on the host from those samples.

### Built-in 100 Hz / 120 Hz classification

The simple flicker detector is separate from the spectral-channel sequence.
It does not need `configureAutoSMUX()`, `setIntegration()`, or spectral gain.

```cpp
#include <AS7343_7Semi.h>

AS7343_7Semi sensor;

void setup() {
  Serial.begin(115200);
  if (!sensor.begin()) {
    while (true) delay(1000);
  }

  sensor.powerOn();
  if (!sensor.enableFlickerDetection()) {
    while (true) delay(1000);
  }
}

void loop() {
  const uint32_t startTime = millis();
  while (!sensor.flickerDetectionValid() &&
         static_cast<uint32_t>(millis() - startTime) < 1500) {
    delay(10);
  }

  if (!sensor.flickerDetectionValid()) {
    Serial.println("Flicker measurement timeout");
  } else if (sensor.flickerDetectionSaturated()) {
    Serial.println("Flicker measurement saturated");
  } else {
    bool detected = false;
    uint16_t frequencyHz = 0;
    if (!sensor.getFlickerInfo(detected, frequencyHz)) {
      Serial.println("Failed to read flicker information");
    } else if (!detected) {
      Serial.println("No supported 100 Hz or 120 Hz classification");
    } else {
      Serial.println(frequencyHz);
    }
  }
  delay(500);
}
```

`getFlickerInfo(detected, frequencyHz)` returns `true` when the status register
was read successfully; `detected` is true and `frequencyHz` is `100` or `120`
only when the sensor reports that supported classification. Otherwise, the
call succeeds with `detected == false` and `frequencyHz == 0`. Check
`flickerDetectionValid()` and `flickerDetectionSaturated()` when interpreting
the result. This API does not calculate arbitrary flicker frequencies.

### FIFO-based frequency estimate

The FIFO mode captures 8-bit flicker samples. Configure it with an integration
time from 1 to 255 ticks; the library enables the 8-bit flicker FIFO, clears
old samples, and starts flicker acquisition. The `FD_TIME` integration-time
formula in the datasheet is 2.78 microseconds per tick. That is not a
guaranteed FIFO sample interval, so the `sampleRateHz` passed to the estimator
must be measured/calibrated on the target hardware for a meaningful estimate.

```cpp
constexpr uint16_t FD_TICKS = 200;
constexpr uint16_t SAMPLE_COUNT = 128;
constexpr float CALIBRATED_SAMPLE_RATE_HZ = 1800.0f; // Replace after calibration.

uint8_t samples[SAMPLE_COUNT];
uint16_t count = 0;

if (!sensor.configureFlickerFifo(FD_TICKS)) {
  Serial.println("Could not configure flicker FIFO");
  return;
}

const uint32_t startTime = millis();
while (count < SAMPLE_COUNT &&
       static_cast<uint32_t>(millis() - startTime) < 1500) {
  uint16_t justRead = 0;
  if (!sensor.readFlickerFifo(samples + count, SAMPLE_COUNT - count, justRead)) {
    Serial.println("Could not read flicker FIFO");
    break;
  }
  count += justRead;
  if (justRead == 0) delay(1);
}
if (count < SAMPLE_COUNT) {
  Serial.println("Flicker FIFO capture timed out");
}

bool overflowed = false;
float estimatedHz = 0.0f;
if (!sensor.flickerFifoOverflowed(overflowed)) {
  Serial.println("Could not read FIFO overflow status");
} else if (overflowed) {
  Serial.println("FIFO overflow: discard the sample window");
} else if (AS7343_7Semi::estimateFlickerFrequencyHz(
               samples, count, CALIBRATED_SAMPLE_RATE_HZ, estimatedHz)) {
  Serial.print("Estimated frequency: ");
  Serial.println(estimatedHz);
}
```

`readFlickerFifo()` drains complete two-byte FIFO entries into the caller's
buffer. Discard a capture if FIFO overflow is reported. The estimator removes
the sample mean, interpolates rising mean crossings, and averages the periods
between them; it requires at least three rising crossings. Its result is an
estimate, not an exact or calibrated measurement, and can be affected by
waveform shape, noise, sample-rate error, and FIFO overflow. The FIFO's
sample cadence and byte chronology should be validated on the actual sensor
board. The integration-time register encoding follows the datasheet's register
map (`0xE0`/`0xE2`); the prose in that section contains a conflicting address
reference. To stop basic flicker detection, call
`enableFlickerDetection(false)`.

## I2C bus and pin options

Use the default bus:

```cpp
sensor.begin();
```

Pass a specific `TwoWire` bus:

```cpp
sensor.begin(Wire);
```

Pass custom pins on ESP32/ESP8266:

```cpp
sensor.begin(21, 22);       // SDA, SCL, default Wire object
sensor.begin(21, 22, Wire); // SDA, SCL, selected TwoWire object
```

On other architectures the pin arguments are ignored and the selected
`TwoWire` instance is started with its normal `begin()` behavior.

## Examples

| Example | Purpose |
|---|---|
| `00_GetRAWValues` | Configure and print the named spectral channels over Serial |
| `01_SpectralMonitor` | ESP32 Wi-Fi access point with live spectral graph, channel table, and LED toggle |
| `02_FlickerFrequencyEstimate` | Capture FIFO samples and estimate flicker frequency |
| `03_VIS` | Print the average of the six VIS/CLEAR readings |

For the Spectral Monitor, connect to the access point configured near the top
of the sketch and browse to the IP address printed to Serial. Change the
example's SSID and password before deployment; the example credentials are
not intended as production security.

## Troubleshooting

- **Initialization fails:** Check sensor power, common ground, SDA/SCL wiring,
  logic-level compatibility, and whether the board responds at I2C address
  `0x59`.
- **Read fails:** Ensure `begin()` succeeded and the sensor remains connected.
  Spectral reads wait up to one second for valid data.
- **All channel values are zero or unexpectedly small:** Call `read()` and
  check its return value before getters. Confirm that 18-channel Auto-SMUX is
  configured and select a suitable gain/integration time.
- **Readings saturate:** Reduce gain and/or integration time, or reduce the
  incident light.
- **No flicker classification:** The built-in detector classifies 100 Hz and
  120 Hz only. Verify that flicker detection is enabled, the sensor is powered,
  the result is valid, and the measurement is not saturated. For other
  frequencies, use the FIFO estimator and calibrate its sample rate.
- **No change when toggling the LED:** The board LED is a separate illumination
  control; verify the board's LED hardware and its power requirements.

## License

MIT. See [LICENSE](./LICENSE).