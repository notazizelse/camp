/*
  =====================================================================
  CubeSat Payload Computer  -  CAMP v2 board
  =====================================================================
  Hardware : Arduino Nano + I2C LCD + DHT11 + MQ-7 + 2 RGB LEDs + 3 buttons
  Board    : campv2.kicad_sch  (pin numbers below were read off the PCB)

  WHAT IT DOES
  ------------
  The LCD shows one "screen" at a time. Press the NEXT button to walk
  through them:

        NAME  ->  TEMPERATURE  ->  HUMIDITY  ->  CARBON MONOXIDE
          ^                                            |
          |____________________________________________|

  The two RGB LEDs are status lights, like on a real satellite:
        LED2 = how warm it is      LED3 = how much CO is in the air

  ---------------------------------------------------------------------
  PIN MAP  (taken from the schematic - do not change these)
  ---------------------------------------------------------------------
    A0  button 1        A4  LCD SDA        D2   DHT11 data
    A1  button 2        A5  LCD SCL        D12  MQ-7 digital alarm
    D8  button 3        A7  MQ-7 analog

    LED2  red D3   green D5    blue D6
    LED3  red D9   green D10   blue D11
    (all six are PWM pins, so the LEDs can fade, not just blink)

  ---------------------------------------------------------------------
  LIBRARIES TO INSTALL  (Arduino IDE -> Tools -> Manage Libraries...)
  ---------------------------------------------------------------------
    1. "LiquidCrystal I2C"        by Frank de Brabander
    2. "DHT sensor library"       by Adafruit
    3. "Adafruit Unified Sensor"  by Adafruit   (comes with #2)

  Board settings:  Tools -> Board -> Arduino Nano
                   Tools -> Processor -> ATmega328P (Old Bootloader)
                            ...try "Old Bootloader" if uploading fails.
  =====================================================================
*/

#include <Wire.h>               // lets the Arduino talk on the I2C bus
#include <LiquidCrystal_I2C.h>  // controls the LCD over I2C
#include <DHT.h>                // reads the DHT11 temperature/humidity sensor


/* =====================================================================
   PART 1 - SETTINGS YOU ARE MEANT TO CHANGE
   =====================================================================
   Everything in this block is safe to edit. Everything below it is
   wiring, so leave that alone.
   ===================================================================== */

// Put your own name here. This is what the LCD shows when it starts up.
const char STUDENT_NAME[] = "YOUR NAME";

// Most I2C LCD modules are 0x27. If your screen stays blank or shows
// only white boxes, change this to 0x3F.
const byte LCD_ADDRESS = 0x27;
const byte LCD_COLUMNS = 16;
const byte LCD_ROWS    = 2;

// The RGB LEDs share one "common" leg, and jumper J5 on the board decides
// whether that leg goes to GND or to +5V.
//     J5 jumper on the GND side (pins 1-2) -> COMMON_ANODE = false
//     J5 jumper on the +5V side (pins 2-3) -> COMMON_ANODE = true
// If your colours come out backwards (red when it should be off), flip this.
const bool COMMON_ANODE = false;

// Show temperature in Fahrenheit instead of Celsius?
// Button 3 flips this while the program is running.
bool useFahrenheit = false;

// Temperature limits for the LED2 status light, in degrees Celsius.
const float TEMP_COLD_C = 18.0;   // below this -> blue
const float TEMP_HOT_C  = 28.0;   // above this -> red

// Carbon monoxide limits for the LED3 status light, in ppm
// (ppm = "parts per million"). Roughly: 35 ppm is the workplace limit
// for 8 hours, 200 ppm gives you a headache.
const float CO_WARNING_PPM = 35.0;
const float CO_DANGER_PPM  = 200.0;

// The MQ-7 sensor has to heat up before its numbers mean anything.
// The screen says "warming up" until this many milliseconds have passed.
const unsigned long MQ7_WARMUP_MS = 20000UL;   // 20 seconds

// MQ-7 calibration. R0 is the sensor's resistance in clean air, in kilo-ohms.
// Every sensor is slightly different. To find yours, set CALIBRATE_MODE to
// true, upload, open the Serial Monitor at 9600, leave the board in fresh
// air for a few minutes, then copy the printed number into MQ7_R0.
const bool  CALIBRATE_MODE = false;
const float MQ7_R0         = 10.0;


/* =====================================================================
   PART 2 - THE WIRING  (these match the PCB, don't edit them)
   ===================================================================== */

const byte PIN_BUTTON_NEXT  = A0;   // SW1
const byte PIN_BUTTON_BACK  = A1;   // SW2
const byte PIN_BUTTON_UNITS = 8;    // SW3

const byte PIN_DHT11 = 2;           // J2 pin 2

const byte PIN_MQ7_ANALOG = A7;     // J4 pin 1 - the actual gas reading
const byte PIN_MQ7_ALARM  = 12;     // J4 pin 2 - module's own alarm output

const byte PIN_LED2_RED   = 3;      // "temperature" light
const byte PIN_LED2_GREEN = 5;
const byte PIN_LED2_BLUE  = 6;

const byte PIN_LED3_RED   = 9;      // "air quality" light
const byte PIN_LED3_GREEN = 10;
const byte PIN_LED3_BLUE  = 11;

// The MQ-7 module has a 10k resistor on it that we measure against.
const float MQ7_LOAD_RESISTOR = 10.0;   // kilo-ohms

// These two numbers come from the graph in the MQ-7 datasheet. They turn
// a resistance ratio into a CO reading in ppm.
const float MQ7_CURVE_A = 99.042;
const float MQ7_CURVE_B = -1.518;


/* =====================================================================
   PART 3 - THE PARTS, AS OBJECTS
   ===================================================================== */

LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLUMNS, LCD_ROWS);
DHT dht(PIN_DHT11, DHT11);


/* =====================================================================
   PART 4 - MEMORY: what the program needs to remember
   ===================================================================== */

// The list of screens. SCREEN_COUNT is not a real screen - it is just
// the Arduino counting them for us, which makes the wrap-around easy.
enum Screen {
  SCREEN_NAME,
  SCREEN_TEMPERATURE,
  SCREEN_HUMIDITY,
  SCREEN_CARBON_MONOXIDE,
  SCREEN_COUNT
};

byte currentScreen = SCREEN_NAME;   // which screen we are looking at now
bool screenChanged = true;          // true = the LCD needs redrawing

// The newest sensor readings live here so every part of the program
// can use the same numbers.
float temperatureC = 0.0;
float humidity     = 0.0;
float coPPM        = 0.0;
bool  sensorsOK    = false;         // false if the DHT11 did not answer
bool  gasAlarm     = false;         // true if the MQ-7 module's alarm tripped

// Timers. We never use delay() in the main loop, because delay() would
// freeze the buttons. Instead we remember "when did I last do this?".
unsigned long lastSensorRead = 0;
unsigned long lastSerialSend = 0;
unsigned long bootTime       = 0;

const unsigned long SENSOR_INTERVAL_MS = 2000;  // DHT11 is slow, 2s minimum
const unsigned long SERIAL_INTERVAL_MS = 1000;

// Button bookkeeping. Three buttons, so three of everything.
const byte BUTTON_COUNT = 3;
const byte buttonPins[BUTTON_COUNT] = {
  PIN_BUTTON_NEXT, PIN_BUTTON_BACK, PIN_BUTTON_UNITS
};
bool          buttonWasDown[BUTTON_COUNT]   = { false, false, false };
unsigned long buttonLastChange[BUTTON_COUNT] = { 0, 0, 0 };

// A button's metal contacts bounce for a few milliseconds when pressed,
// which the Arduino would read as many presses. We ignore anything that
// happens within this window. This is called "debouncing".
const unsigned long DEBOUNCE_MS = 40;


/* =====================================================================
   PART 5 - SETUP: runs once, when the board powers on
   ===================================================================== */

void setup() {
  Serial.begin(9600);
  Serial.println(F("CubeSat booting..."));

  // Buttons: one leg goes to the pin, the other goes to GND.
  // INPUT_PULLUP switches on a resistor inside the chip that holds the
  // pin HIGH. Pressing the button drags it down to GND, so:
  //        NOT pressed = HIGH        pressed = LOW
  for (byte i = 0; i < BUTTON_COUNT; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }

  // The six LED legs are outputs.
  pinMode(PIN_LED2_RED,   OUTPUT);
  pinMode(PIN_LED2_GREEN, OUTPUT);
  pinMode(PIN_LED2_BLUE,  OUTPUT);
  pinMode(PIN_LED3_RED,   OUTPUT);
  pinMode(PIN_LED3_GREEN, OUTPUT);
  pinMode(PIN_LED3_BLUE,  OUTPUT);

  // The MQ-7's alarm pin is an input, with the pull-up resistor switched on.
  // The pull-up matters: if nobody has plugged a sensor into J4 yet, a bare
  // input pin picks up electrical noise from the air and flickers randomly,
  // which shows up as a fake "GAS ALARM". The pull-up holds the pin HIGH
  // (= no alarm) until a real sensor is connected to pull it LOW.
  //
  // NOTE: we do NOT call pinMode on A7. On a Nano, A6 and A7 can only be
  // read with analogRead - they have no digital circuitry at all.
  pinMode(PIN_MQ7_ALARM, INPUT_PULLUP);

  lcd.init();
  lcd.backlight();
  dht.begin();

  bootTime = millis();

  showSplashScreen();     // "Made by <name>"
  ledSelfTest();          // flash both LEDs so we can see they work

  if (CALIBRATE_MODE) {
    runCalibration();
  }
}


/* =====================================================================
   PART 6 - LOOP: runs over and over, thousands of times a second
   =====================================================================
   Notice how short this is. Each line is one job, and the job itself
   is written out further down. This is how you keep a program readable.
   ===================================================================== */

void loop() {
  handleButtons();      // did anyone press anything?
  readSensors();        // every 2 seconds, ask the sensors for numbers
  updateStatusLEDs();   // colour the two LEDs to match the readings
  updateDisplay();      // draw the current screen
  sendToSerial();       // every 1 second, report to the "ground station"
}


/* =====================================================================
   PART 7 - THE BUTTONS
   ===================================================================== */

// Returns true only on the single moment a button goes from up to down.
// Holding the button down does NOT keep returning true.
bool wasJustPressed(byte index) {
  bool isDownNow = (digitalRead(buttonPins[index]) == LOW);

  // Still bouncing? Too soon to trust the reading, so say "no press".
  if (millis() - buttonLastChange[index] < DEBOUNCE_MS) {
    return false;
  }

  // Nothing changed since last time -> no new press.
  if (isDownNow == buttonWasDown[index]) {
    return false;
  }

  // Something changed. Remember it, and note the time.
  buttonWasDown[index]    = isDownNow;
  buttonLastChange[index] = millis();

  return isDownNow;   // true if the change was up -> down
}

void handleButtons() {

  // ---- Button 1 (SW1, pin A0): go to the NEXT screen ----
  if (wasJustPressed(0)) {
    currentScreen = currentScreen + 1;
    if (currentScreen >= SCREEN_COUNT) {
      currentScreen = SCREEN_NAME;      // ran off the end, wrap to the start
    }
    screenChanged = true;
  }

  // ---- Button 2 (SW2, pin A1): go BACK one screen ----
  if (wasJustPressed(1)) {
    if (currentScreen == SCREEN_NAME) {
      currentScreen = SCREEN_COUNT - 1; // ran off the front, wrap to the end
    } else {
      currentScreen = currentScreen - 1;
    }
    screenChanged = true;
  }

  // ---- Button 3 (SW3, pin D8): swap Celsius <-> Fahrenheit ----
  if (wasJustPressed(2)) {
    useFahrenheit = !useFahrenheit;    // "!" means "the opposite of"
    screenChanged = true;
  }
}


/* =====================================================================
   PART 8 - THE SENSORS
   ===================================================================== */

void readSensors() {
  // Has it been 2 seconds yet? If not, leave immediately.
  if (millis() - lastSensorRead < SENSOR_INTERVAL_MS) {
    return;
  }
  lastSensorRead = millis();

  float newTemp     = dht.readTemperature();   // always Celsius
  float newHumidity = dht.readHumidity();

  // isnan() means "is not a number". The DHT11 returns that when the
  // reading failed - usually a loose wire on connector J2.
  if (isnan(newTemp) || isnan(newHumidity)) {
    sensorsOK = false;
  } else {
    sensorsOK     = true;
    temperatureC  = newTemp;
    humidity      = newHumidity;
  }

  coPPM    = readCarbonMonoxide();
  gasAlarm = (digitalRead(PIN_MQ7_ALARM) == LOW);  // module pulls LOW on alarm

  screenChanged = true;   // new numbers, so the LCD needs redrawing
}

// Turns the MQ-7's analog reading into an estimate in ppm.
//
// How it works, in three steps:
//   1. analogRead gives 0..1023. Turn that into volts.
//   2. The sensor and a 10k resistor form a voltage divider, so from the
//      volts we can work out the sensor's own resistance (Rs).
//   3. Compare Rs to R0 (its resistance in clean air) and look the answer
//      up on the datasheet's curve.
float readCarbonMonoxide() {
  int   raw   = analogRead(PIN_MQ7_ANALOG);
  float volts = raw * (5.0 / 1023.0);

  if (volts < 0.01) {
    volts = 0.01;      // never divide by zero
  }

  float sensorResistance = ((5.0 - volts) / volts) * MQ7_LOAD_RESISTOR;
  float ratio            = sensorResistance / MQ7_R0;
  float ppm              = MQ7_CURVE_A * pow(ratio, MQ7_CURVE_B);

  if (ppm < 0)     ppm = 0;
  if (ppm > 10000) ppm = 10000;   // beyond this the sensor is guessing

  return ppm;
}

bool sensorIsWarmedUp() {
  return (millis() - bootTime) > MQ7_WARMUP_MS;
}

// Only used when CALIBRATE_MODE is true at the top of the file.
void runCalibration() {
  lcd.clear();
  showLine(0, "Calibrating...");
  showLine(1, "Fresh air only!");

  float total = 0;
  for (int i = 0; i < 100; i++) {
    int   raw   = analogRead(PIN_MQ7_ANALOG);
    float volts = raw * (5.0 / 1023.0);
    if (volts < 0.01) volts = 0.01;
    total += ((5.0 - volts) / volts) * MQ7_LOAD_RESISTOR;
    delay(50);           // delay() is fine here - we are not in loop() yet
  }

  // In clean air the datasheet says Rs is about 27 times R0.
  float measuredR0 = (total / 100.0) / 27.0;

  Serial.println(F("---- MQ-7 CALIBRATION ----"));
  Serial.print(F("Copy this into MQ7_R0:  "));
  Serial.println(measuredR0, 3);
  Serial.println(F("--------------------------"));

  lcd.clear();
  showLine(0, "R0 = " + String(measuredR0, 2));
  showLine(1, "See Serial Mon.");
  delay(4000);
}


/* =====================================================================
   PART 9 - THE LCD
   ===================================================================== */

// Writes one line and pads it with spaces out to the edge of the screen.
// Without the padding, leftovers from the last screen stay on display -
// e.g. "24.5 C" written over "100.0 %" would leave a stray "%".
void showLine(byte row, String text) {
  while (text.length() < LCD_COLUMNS) {
    text += ' ';
  }
  lcd.setCursor(0, row);
  lcd.print(text.substring(0, LCD_COLUMNS));
}

void showSplashScreen() {
  lcd.clear();
  showLine(0, "Made by");
  showLine(1, STUDENT_NAME);
  delay(2500);          // delay() is OK in setup() - nothing else to do yet
  screenChanged = true;
}

void updateDisplay() {
  // Nothing new to show? Then don't touch the LCD. Redrawing an unchanged
  // screen over and over makes it flicker.
  if (!screenChanged) {
    return;
  }
  screenChanged = false;

  switch (currentScreen) {

    case SCREEN_NAME:
      showLine(0, "Made by");
      showLine(1, STUDENT_NAME);
      break;

    case SCREEN_TEMPERATURE:
      showLine(0, "Temperature");
      if (!sensorsOK) {
        showLine(1, "sensor error");
      } else if (useFahrenheit) {
        showLine(1, String(temperatureC * 9.0 / 5.0 + 32.0, 1) + " F");
      } else {
        showLine(1, String(temperatureC, 1) + " C");
      }
      break;

    case SCREEN_HUMIDITY:
      showLine(0, "Humidity");
      if (!sensorsOK) {
        showLine(1, "sensor error");
      } else {
        showLine(1, String(humidity, 0) + " %RH");
      }
      break;

    case SCREEN_CARBON_MONOXIDE:
      showLine(0, "Carbon Monoxide");
      if (!sensorIsWarmedUp()) {
        showLine(1, "warming up...");
      } else if (gasAlarm) {
        showLine(1, String(coPPM, 0) + " ppm ALARM");
      } else {
        showLine(1, String(coPPM, 0) + " ppm");
      }
      break;
  }
}


/* =====================================================================
   PART 10 - THE TWO RGB LEDs
   ===================================================================== */

// Sets one RGB LED to a colour. Each of red, green and blue is a number
// from 0 (off) to 255 (full brightness), so mixing them gives any colour.
//
// The COMMON_ANODE flip: when the common leg is tied to +5V, current
// flows when the Arduino pin is LOW. So the brightness numbers have to
// be turned upside down - 255 becomes 0, 0 becomes 255.
void setLedColour(byte redPin, byte greenPin, byte bluePin,
                  byte red,    byte green,    byte blue) {
  if (COMMON_ANODE) {
    red   = 255 - red;
    green = 255 - green;
    blue  = 255 - blue;
  }
  analogWrite(redPin,   red);
  analogWrite(greenPin, green);
  analogWrite(bluePin,  blue);
}

void updateStatusLEDs() {

  // ---- LED2: temperature ----
  if (!sensorsOK) {
    setLedColour(PIN_LED2_RED, PIN_LED2_GREEN, PIN_LED2_BLUE, 40, 0, 40); // purple = broken
  } else if (temperatureC < TEMP_COLD_C) {
    setLedColour(PIN_LED2_RED, PIN_LED2_GREEN, PIN_LED2_BLUE, 0, 0, 255); // blue = cold
  } else if (temperatureC > TEMP_HOT_C) {
    setLedColour(PIN_LED2_RED, PIN_LED2_GREEN, PIN_LED2_BLUE, 255, 0, 0); // red = hot
  } else {
    setLedColour(PIN_LED2_RED, PIN_LED2_GREEN, PIN_LED2_BLUE, 0, 255, 0); // green = just right
  }

  // ---- LED3: carbon monoxide ----
  if (!sensorIsWarmedUp()) {
    setLedColour(PIN_LED3_RED, PIN_LED3_GREEN, PIN_LED3_BLUE, 0, 0, 60);  // dim blue = warming up
  } else if (coPPM > CO_DANGER_PPM || gasAlarm) {
    setLedColour(PIN_LED3_RED, PIN_LED3_GREEN, PIN_LED3_BLUE, 255, 0, 0); // red = danger
  } else if (coPPM > CO_WARNING_PPM) {
    setLedColour(PIN_LED3_RED, PIN_LED3_GREEN, PIN_LED3_BLUE, 255, 150, 0); // orange = warning
  } else {
    setLedColour(PIN_LED3_RED, PIN_LED3_GREEN, PIN_LED3_BLUE, 0, 255, 0);  // green = clean air
  }
}

// Runs once at startup: red, green, blue on both LEDs, so you can see at
// a glance that all six legs are soldered properly.
void ledSelfTest() {
  byte testColours[3][3] = {
    { 255, 0,   0   },
    { 0,   255, 0   },
    { 0,   0,   255 }
  };

  for (byte i = 0; i < 3; i++) {
    setLedColour(PIN_LED2_RED, PIN_LED2_GREEN, PIN_LED2_BLUE,
                 testColours[i][0], testColours[i][1], testColours[i][2]);
    setLedColour(PIN_LED3_RED, PIN_LED3_GREEN, PIN_LED3_BLUE,
                 testColours[i][0], testColours[i][1], testColours[i][2]);
    delay(300);
  }

  setLedColour(PIN_LED2_RED, PIN_LED2_GREEN, PIN_LED2_BLUE, 0, 0, 0);
  setLedColour(PIN_LED3_RED, PIN_LED3_GREEN, PIN_LED3_BLUE, 0, 0, 0);
}


/* =====================================================================
   PART 11 - TELEMETRY
   =====================================================================
   A real CubeSat radios its readings down to a ground station. Ours
   sends them down the USB cable. Open Tools -> Serial Monitor (9600) to
   watch, or Tools -> Serial Plotter to see them drawn as graphs.
   ===================================================================== */

void sendToSerial() {
  if (millis() - lastSerialSend < SERIAL_INTERVAL_MS) {
    return;
  }
  lastSerialSend = millis();

  Serial.print(F("T="));
  Serial.print(temperatureC, 1);
  Serial.print(F("C  RH="));
  Serial.print(humidity, 0);
  Serial.print(F("%  CO="));
  Serial.print(coPPM, 0);
  Serial.print(F("ppm"));

  if (!sensorIsWarmedUp()) Serial.print(F("  [warming up]"));
  if (gasAlarm)            Serial.print(F("  [GAS ALARM]"));
  if (!sensorsOK)          Serial.print(F("  [DHT11 ERROR]"));

  Serial.println();
}