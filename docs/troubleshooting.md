# Troubleshooting

Work down the list. Each section starts with the cause that turns out to be right most of
the time.

---

## Nothing happens at all

**No light anywhere, nothing on the screen.**

1. **Is the Nano actually in its socket, the right way round?** The `USB` outline is
   printed on the silkscreen at the bottom edge of the board. The USB connector goes
   there.
2. **Is the board getting 5 V?** Measure between the `VCC` and `GND` pins of any module
   header. Expect 4.8 – 5.2 V.
3. **Solder bridge across the Arduino socket.** Those 30 pins are the tightest spacing on
   the board. Look along the back at a shallow angle, in good light.
4. **The Nano itself is dead.** Unplug it from the board entirely and plug it into your
   computer with USB. Its own power LED should light. If it does not, the Nano is the
   problem, not your soldering.

---

## The upload fails

### `avrdude: stk500_recv(): programmer is not responding`

This is the single most common problem and it is almost never a fault.

Change **Tools → Processor** to **ATmega328P (Old Bootloader)** and upload again. Most
inexpensive Nano clones ship with the older bootloader, and the IDE defaults to the new
one.

### `Port not found` / no port in the Tools menu

The CH340 USB-to-serial chip used on clone Nanos needs a driver on Windows and older
macOS. Install the CH340 driver, unplug, replug.

Also check your USB cable — a surprising number of cheap cables are charge-only and carry
no data lines at all. Try a different one.

### Upload starts and then fails partway

Unplug everything from the module headers and try again. A sensor drawing a lot of current
during the upload can brown out the board.

---

## The screen

### Backlight is on, but the display is blank or shows a row of white blocks

**Wrong I²C address.** Change `LCD_ADDRESS` in the sketch:

```cpp
const byte LCD_ADDRESS = 0x3F;   // was 0x27
```

The two common addresses for these backpacks are `0x27` and `0x3F`, and there is no way to
tell from the outside which one you have.

If neither works, upload an I²C scanner sketch and read the address it finds.

### Backlight is on, contrast looks wrong — very faint or solid blocks

There is a small blue potentiometer on the I²C backpack, on the back of the display.
Turn it slowly with a screwdriver while the board is running.

### Nothing at all, not even a backlight

Check the module's 4 pins against the header: `GND`, `VCC`, `SDA`, `SCL`, counting from the
top of the board. If it is plugged in one position off, or backwards, nothing happens.

### The screen works but the text is garbled or freezes

Usually a loose `SDA` or `SCL` joint. Reflow the four pins of the `LCD` header.

---

## The LEDs

### They are lit when they should be dark, and the colours look inverted

The `J5` jumper and the `COMMON_ANODE` setting disagree with each other.

| Your LEDs | Jumper on `J5` | Sketch |
|---|---|---|
| common cathode | `−` side (pins 1–2) | `COMMON_ANODE = false` |
| common anode | `+` side (pins 2–3) | `COMMON_ANODE = true` |

Change **one** of the two — not both.

### One colour never lights

1. **The LED is rotated.** Pull it out and check the long leg is in position 2 of the
   socket.
2. **The resistor for that colour has a bad joint.** `R4`/`R5`/`R6` serve `LED1`;
   `R7`/`R8`/`R9` serve `LED2`. Reflow both ends.
3. **Test it in software** — set that one pin high in `setup()` and see if the colour
   appears.

### No LED lights at all

Is the jumper cap on `J5`? Without it, the LEDs' common leg is not connected to anything
and neither LED can light. This one catches everybody once.

---

## The sensors

### Temperature and humidity read `nan`, or the screen says the sensor failed

1. **DHT11 plugged in backwards.** The three pins are `VCC`, `DATA`, `GND` counting from
   the top of the board.
2. **The middle pin is not making contact.** These modules have long thin pins that bend
   easily.
3. **You are reading it too fast.** A DHT11 needs about 2 seconds between readings. The
   supplied sketch already respects this; if you have edited `SENSOR_INTERVAL_MS`, put it
   back to 2000 or more.

### The CO reading is nonsense, or wildly high in clean air

1. **Give it time.** The MQ-7 has an internal heater that must come up to temperature.
   The screen says `warming up` for the first 20 seconds; the reading is meaningless before
   that, and for a brand-new sensor the first *few hours* of use are unreliable while it
   burns in.
2. **It is not calibrated.** `MQ7_R0` defaults to 10.0 kΩ, which is a guess. Follow the
   calibration steps in the [README](../README.md#calibrating-the-mq-7).
3. **The module is plugged in wrong.** Pin 1 of the `MQ-7` header goes to `A7` and must
   receive the module's **analogue** output; pin 2 goes to `D12` and takes the digital one.
   MQ-7 breakout boards differ between suppliers — read the labels printed on your module
   rather than trusting the pin order. (The pin legend on the schematic itself has these
   two swapped — see [hardware-reference.md §7](hardware-reference.md#7-known-issues).)

### `GAS ALARM` appears with no sensor plugged in

It should not — `D12` is configured with its internal pull-up for exactly this reason. If
it happens anyway, check for a solder bridge near the `MQ-7` header.

---

## The buttons

### Nothing happens when I press

1. **Wrong button.** `SW1` = next screen, `SW2` = previous, `SW3` = °C/°F. They are marked
   on the silkscreen.
2. **A cold joint on the switch.** Reflow all four legs.
3. **The switch is rotated 90°.** A 4-leg tactile switch fits the board in two
   orientations, but only one of them connects the pair the board uses. If it went in
   without any force, it may be the wrong way. Desolder, rotate 90°, refit.

### One press registers as several

The debounce window is too short for your switches. Increase it in the sketch:

```cpp
const unsigned long DEBOUNCE_MS = 60;   // was 40
```

---

## Power

### The Nano gets hot

Disconnect immediately.

- **USB and the screw terminal are both connected.** This board has no protection between
  them. Use one or the other.
- **The screw terminal polarity is reversed.** `+` and `−` are marked on the back.
- **There is a short between 5 V and GND.** The front of the board is a +5 V plane and the
  back is a GND plane, so a stray leg or a blob can bridge them easily. Measure resistance
  between a `VCC` and a `GND` pin with the power off — anything under about 100 Ω means
  you have a short.

### The screen dims or the board resets when the MQ-7 warms up

The MQ-7's heater draws around 150 mA. If you are powering everything from a laptop USB
port through a long thin cable, you can run out of headroom. Use the screw terminal with a
proper 5 V supply, or a shorter cable.

---

## Still stuck

Open the Serial Monitor at **9600 baud** and watch what the board says as it boots. It
prints `CubeSat booting...`, then reports readings once a second. That output usually
tells you which stage is failing.

If you think you have found a fault in the board rather than in your build, please open an
issue with a photo of both sides of your assembled board.
