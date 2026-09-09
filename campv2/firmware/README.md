# Firmware

## `CubeSat/CubeSat.ino`

The sketch that runs on the board. Open it in the Arduino IDE, change `STUDENT_NAME`, and
upload. Full instructions are in the [main README](../../README.md#flash-it).

The sketch is written to be read: it is divided into numbered parts, the settings you are
meant to change are all at the top, and the pin assignments below them match the copper on
the board.

## Libraries

The sketch needs three Arduino libraries:

| Library | Author |
|---|---|
| LiquidCrystal I2C | Frank de Brabander |
| DHT sensor library | Adafruit |
| Adafruit Unified Sensor | Adafruit |

**Normally you install them the usual way**, through **Tools → Manage Libraries…** in the
Arduino IDE, which gets you the current versions with their updates.

For a classroom with no internet, known-good copies are vendored at the top of the
repository in [`Arduino/libraries/`](../../Arduino/libraries/). Close the Arduino IDE,
copy those three folders into your sketchbook's `libraries` directory, and reopen it:

| Platform | Where to copy them |
|---|---|
| Windows | `Documents\Arduino\libraries\` |
| macOS | `~/Documents/Arduino/libraries/` |
| Linux | `~/Arduino/libraries/` |

Each library keeps its own licence file; they are third-party code, vendored only for
convenience.

## Using PlatformIO instead

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

PlatformIO resolves the libraries itself, so the vendored copies are not used.
