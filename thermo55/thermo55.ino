#include <Adafruit_MAX31855.h>

#include "thermo55.h"
#include "thermo55_radio.h"

// above highest reading for K-type thermocouple
#define MAX_TEMP 99999
// below lowest reading for K-type thermocouple
#define MIN_TEMP -99999

// LCD I2C address and size
const PROGMEM uint8_t LCD_I2C_ADDR = 0x27;
const PROGMEM uint8_t LCD_WIDTH = 16;
const PROGMEM uint8_t LCD_HEIGHT = 2;

// alert output
const PROGMEM uint8_t PIN_OUT = 3;

// inverted alert output
const PROGMEM uint8_t PIN_OUT_ = A3;

// switch lcd display mode (normal or max/min)
const PROGMEM uint8_t PIN_BUTTON_ = 5;

// wire to GND to keep display permanently on
const PROGMEM uint8_t PIN_ALWAYS_ON_ = 6;

// If wired to ground, display temp in degrees Fahrenheit on LCD
const PROGMEM uint8_t PIN_DISP_F_ = 2;

// If wired to ground, alarm on low temp. Else alarm on high temp.
const PROGMEM uint8_t PIN_ALARM_DIR = 4;

// If wired to ground, receiver mode, else transmit mode
const PROGMEM uint8_t PIN_XMIT = A1;

bool xmitMode;

// analog input to set alarm threshold
const PROGMEM uint8_t PIN_THRESHOLD_COARSE = A7;
const PROGMEM uint8_t PIN_THRESHOLD_FINE = A6;

// SPI hardware configuration
const PROGMEM uint8_t PIN_thermoDO = 12; // MISO
const PROGMEM uint8_t PIN_thermoCS = 8;  // Chip select
const PROGMEM uint8_t PIN_thermoCLK = 7; // SPI serial clock (NOTE must not be the same as the nRF24L01 clock)

Adafruit_MAX31855 thermocouple(PIN_thermoCLK, PIN_thermoCS, PIN_thermoDO);

const PROGMEM int BAUD_RATE = 9600;

// time in seconds to display live data before hiding. Must be <= 255
const PROGMEM uint8_t DISPLAY_TIME = 30;

bool alwaysOnDisplay = false;
bool prevButton = false;
bool maxMinDisplay = false;

float maxTemp;
float minTemp;

// seconds of display left
uint8_t displayCountdown;

LiquidCrystal_I2C lcd(LCD_I2C_ADDR, LCD_WIDTH, LCD_HEIGHT);

void turnOnDisplay() {
  displayCountdown = DISPLAY_TIME;
}

void setOutput(bool x) {
  digitalWrite(PIN_OUT, x ? HIGH : LOW);
  digitalWrite(PIN_OUT_, x ? LOW : HIGH);
}

float getThreshold() {

  // default threshold is below lowest possible reading
  float result = MIN_TEMP;

  int coarse = analogRead(PIN_THRESHOLD_COARSE);
  int fine = analogRead(PIN_THRESHOLD_FINE);
  Serial.print(F("Coarse:  "));
  Serial.println(coarse);
  Serial.print(F("Fine:    "));
  Serial.println(fine);

  if (coarse == 0 || fine == 0) {
    Serial.println(F("Threshold control at zero. Alarm disabled."));
  } else {
    float coarseOffset = 0;
    if (coarse < 512) {
      coarseOffset = -40.0;
    }
    // coarse set to minimum -40C, or +0C based on control position
    result = coarseOffset + (float)coarse * 40.0 / 1023.0;

    // add fine tune -/+5C based on fine 12-bit control
    result += -5.0 + (float)fine * 10.0 / 4095.0;
  }
  return result;
}

void printTemperature(float t) {
  if (t == MAX_TEMP) {
    Serial.println(F("Infinity"));
  } else if (t == MIN_TEMP) {
    Serial.println(F("-Infinity"));
  } else {
    Serial.print(t);
  }
}

float toFahrenheit(float c) {
  if (c >= MAX_TEMP) {
    return c;
  }
  return (c * 1.8) + 32;
}

float tempToDisplay(float t) {
  if (!digitalRead(PIN_DISP_F_)) {
    return toFahrenheit(t);
  }
  return t;
}

void setup() {

  Serial.begin(BAUD_RATE);

  pinMode(PIN_OUT, OUTPUT);
  pinMode(PIN_OUT_, OUTPUT);
  pinMode(PIN_THRESHOLD_FINE, INPUT);
  pinMode(PIN_THRESHOLD_COARSE, INPUT);
  pinMode(PIN_ALARM_DIR, INPUT_PULLUP);
  pinMode(PIN_BUTTON_, INPUT_PULLUP);
  pinMode(PIN_XMIT, INPUT_PULLUP);
  pinMode(PIN_ALWAYS_ON_, INPUT_PULLUP);
  pinMode(PIN_DISP_F_, INPUT_PULLUP);

  setOutput(LOW);

  lcd.init();
  lcd.backlight();
  lcd.clear();

  xmitMode = digitalRead(PIN_XMIT);

  Serial.print(F("Configured as "));
  if (xmitMode) {
    Serial.println(F("transmitter"));
  } else {
    Serial.println(F("receiver"));
  }

  Serial.println();

  maxTemp = MIN_TEMP;
  minTemp = MAX_TEMP;

  displayCountdown = DISPLAY_TIME;
  turnOnDisplay();
  maxMinDisplay = false;

  // wait for MAX31855 to stabilize
  delay(500);

  setupRadio(xmitMode);
}

void checkThermocouple() {

  uint8_t error = thermocouple.readError();
  if (error) {

    lcd.clear();
    lcd.print(F("ERROR"));
    lcd.setCursor(0, 1);

    Serial.print("Error: ");
    if (error & MAX31855_FAULT_OPEN) {
      Serial.println(F("Open Circuit!"));
      lcd.print(F("OPEN CIRCUIT"));
    } else if (error & MAX31855_FAULT_SHORT_GND) {
      Serial.println(F("Short to GND!"));
      lcd.print(F("SHORT TO GND"));
    } else if (error & MAX31855_FAULT_SHORT_VCC) {
      Serial.println(F("Short to VCC!"));
      lcd.print(F("SHORT TO VCC"));
    } else {
      Serial.println(F("Something wrong, other fault"));
      lcd.print(F("UNKNOWN ERROR"));
    }
  }
}

void loop() {

  alwaysOnDisplay = !digitalRead(PIN_ALWAYS_ON_);

  lcd.clear();

  bool button = !digitalRead(PIN_BUTTON_);

  if (button) {
    turnOnDisplay();
    if (maxMinDisplay) {
      // button pressed during max/min display; reset values
      maxTemp = MIN_TEMP;
      minTemp = MAX_TEMP;
    }
  }
  if (button && prevButton) {
    // button held down for 2 samples; switch to max/min
    maxMinDisplay = true;
  }
  prevButton = button;

  float c = MAX_TEMP;

  if (xmitMode) {
    // Transmitter / thermocouple node: always update its own temperature,
    // and answer RF requests using the robust ACK-payload protocol.
    checkThermocouple();
    c = thermocouple.readCelsius();
    if (isRadioEnabled()) {
      processRadioAsSlave(c);
    }
  } else {
    // Receiver / display node: actively request the remote temperature.
    if (isRadioEnabled()) {
      float remote;
      if (requestCelsius(remote)) {
        c = remote;
      }
    }
  }

  if (c > maxTemp && c < MAX_TEMP) {
    maxTemp = c;
  }
  if (c < minTemp) {
    minTemp = c;
  }

  if (displayCountdown > 0) {
    displayCountdown--;
    lcd.display();
    lcd.backlight();
  } else {
    if (!alwaysOnDisplay) {
      lcd.noDisplay();
      lcd.noBacklight();
    }
    maxMinDisplay = false;
  }

  if (!xmitMode || !isRadioEnabled()) {
    float threshold = getThreshold();

    bool alarmOnHighTemp = digitalRead(PIN_ALARM_DIR);

    bool alarm = (alarmOnHighTemp && c >= threshold && c < MAX_TEMP) ||
                 (!alarmOnHighTemp && c <= threshold);
    setOutput(alarm);
    if (alarm) {
      Serial.println(F("ALARM ON"));
      turnOnDisplay();
      maxMinDisplay = false;
    } else {
      Serial.println(F("ALARM OFF"));
    }

    if (!maxMinDisplay) {
      Serial.print(F("Threshold:   "));
      Serial.println(threshold);
      Serial.print(F("Temperature: "));
      printTemperature(c);
      Serial.println();
      Serial.println();

      lcd.setCursor(0, 0);
      if (alarmOnHighTemp) {
        lcd.print(F(">="));
      } else {
        lcd.print(F("<="));
      }
      if (threshold >= 0 && threshold < 10.0) {
        lcd.print(F(" "));
      }
      lcd.print(tempToDisplay(threshold));
    }
  }

  if (!maxMinDisplay) { // normal mode

    if (c < MAX_TEMP) {
      Serial.print(F("Temperature: "));
      printTemperature(c);

      Serial.println();
      lcd.setCursor(0, 1);
      lcd.print(F("TEMP "));
      if (c >= 0 && c < 10.0) {
        lcd.print(F(" "));
      }
      lcd.print(tempToDisplay(c));
    }

  } else { // Max/Min mode

    lcd.setCursor(0, 0);

    if (!(maxTemp > MIN_TEMP)) {
      Serial.println(F("No max/min data"));
      lcd.print(F("NO MAX/MIN DATA"));

    } else {

      lcd.print(F("MAX "));
      Serial.print(F("Maximum since last display: "));
      Serial.println(maxTemp);
      lcd.print(tempToDisplay(maxTemp));

      lcd.setCursor(0, 1);
      lcd.print(F("MIN "));
      Serial.print(F("Minimum since last display: "));
      Serial.println(minTemp);
      lcd.print(tempToDisplay(minTemp));

      Serial.println();
    }
  }
  delay(1000);
}
