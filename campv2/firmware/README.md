# Firmware

## `CubeSat/CubeSat.ino`

The sketch that runs on the board. Open it in the Arduino IDE, change `STUDENT_NAME`, and
upload. Full instructions are in the [main README](../README.md#flash-it).

The sketch is written to be read: it is divided into numbered parts, the settings you are
meant to change are all at the top, and the pin assignments below them match the copper on
the board.

## `libraries/`

Copies of the three Arduino libraries the sketch needs:

| Folder | Library | Author |
|---|---|---|
| `Adafruit_Unified_Sensor/` | Adafruit Unified Sensor | Adafruit |
| `DHT_sensor_library/` | DHT sensor library | Adafruit |
| `LiquidCrystal_I2C/` | LiquidCrystal I2C | Frank de Brabander |

**Normally you do not need these.** Install the libraries the usual way, through
**Tools → Manage Libraries…** in the Arduino IDE, and you will get the current versions
with their updates.

These copies exist for one situation: a classroom with no internet. If the Library Manager
cannot reach the network, close the Arduino IDE, copy these three folders into your
sketchbook's `libraries` directory, and reopen it.

| Platform | Where to copy them |
|---|---|
| Windows | `Documents\Arduino\libraries\` |
| macOS | `~/Documents/Arduino/libraries/` |
| Linux | `~/Arduino/libraries/` |

Each library keeps its own licence file; they are third-party code, vendored here only for
convenience.

## Using PlatformIO instead

If you prefer PlatformIO, the equivalent `platformio.ini` is:

```ini
[env:nanoatmega328]
platform = atmelavr
board = nanoatmega328          ; use nanoatmega328new for the new bootloader
framework = arduino
monitor_speed = 9600
lib_deps =
    marcoschwartz/LiquidCrystal_I2C
    adafruit/DHT sensor library
    adafruit/Adafruit Unified Sensor
```

PlatformIO resolves the libraries itself, so `libraries/` is not used in that case.
