#include "Adafruit_MAX31855.h"

#include "thermo55.h"
#include "thermo55_radio.h"


// LCD I2C address and size
const PROGMEM uint8_t LCD_I2C_ADDR = 0x27;
const PROGMEM uint8_t LCD_WIDTH = 16;
const PROGMEM uint8_t LCD_HEIGHT = 2;

// alert output
const PROGMEM uint8_t PIN_OUT = 3;
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
const PROGMEM uint8_t PIN_thermoCS = 8; // Chip select
const PROGMEM uint8_t PIN_thermoCLK = 7; // SPI serial clock (NOTE must not be the same as the nRF24L01 clock)

Adafruit_MAX31855 thermocouple(PIN_thermoCLK, PIN_thermoCS, PIN_thermoDO);

// Connect LCD I2C pin SDA to A4
// Connect LCD I2C pin SCL to A5
LiquidCrystal_I2C lcd(LCD_I2C_ADDR, LCD_WIDTH, LCD_HEIGHT);

// track max and min temp since last measurement
float maxTemp = MIN_TEMP;
float minTemp = MAX_TEMP;

// time display stays on normally (can be set at compile time)
#ifndef DISPLAY_TIME
  #define DISPLAY_TIME 10
#endif

// countdown time for display
uint32_t displayCountdown;

// display max/min mode
bool maxMinDisplay;

int prevButton = HIGH;

bool alwaysOnDisplay;

void setOutput(bool value) {
  digitalWrite(PIN_OUT, value);
  digitalWrite(PIN_OUT_, !value);
}

void turnOnDisplay() {
  displayCountdown = DISPLAY_TIME;
}

float celsiusToFahrenheit(float celsius) {
  return celsius * 9.0 / 5.0 + 32;
}

float tempToDisplay(float celsius) {
  return !digitalRead(PIN_DISP_F_) ? celsiusToFahrenheit(celsius) : celsius;
}

float prevThreshold = MIN_TEMP;

// alarm threshold supported range in degrees C
// (Note: Type K thermocouple actually supports -200 to +1350)
const float THRESHOLD_COARSE_LOW = -100.0;
const float THRESHOLD_COARSE_HIGH = 300.0;
const float THRESHOLD_FINE_LOW = -10.0;
const float THRESHOLD_FINE_HIGH = 10.0;
const float POT_NOISE_ALLOWANCE = 1.0;

float getThreshold() {
  int reading_coarse = analogRead(PIN_THRESHOLD_COARSE);
  int reading_fine = analogRead(PIN_THRESHOLD_FINE);

  //coarse -> integer -100 - +300 in increments of quantum
  // (Round to multiple of quantum to reduce POT noise)
  float threshold_coarse = THRESHOLD_COARSE_LOW + (THRESHOLD_COARSE_HIGH - THRESHOLD_COARSE_LOW) * reading_coarse / 1023;

  float delta_fine = THRESHOLD_FINE_LOW + (THRESHOLD_FINE_HIGH - THRESHOLD_FINE_LOW) * reading_fine / 1023;
  float threshold = threshold_coarse + delta_fine;

  if (abs(threshold - prevThreshold) > POT_NOISE_ALLOWANCE) {
    turnOnDisplay();
  }
  prevThreshold = threshold;

  return threshold;
}

// never returns
void blinkLED(int millis) {
  while (true) {
    setOutput(HIGH);
    delay(millis);
    setOutput(LOW);
    delay(millis);
  }
}

void errExit() {
  Serial.println(F("Radio fault"));
  lcd.clear();
  lcd.print(F("RADIO FAULT"));
  blinkLED(250);
}

const int BAUD_RATE = 9600;

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
      Serial.println(F("Unspecified error"));
      lcd.print(F("UNSPECIFIED"));
    }
    blinkLED(500);
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
    checkThermocouple();
    c = thermocouple.readCelsius();
    if (isRadioEnabled()) {
      serviceTemperatureRequests(c);
    }
  } else {
    if (requestCelsius(&c)) {
      // got a fresh reading
    } else {
      c = MAX_TEMP;
    }
  }

  if (c > maxTemp && c < MAX_TEMP) {
    maxTemp = c;
  }
  if (c < minTemp) {
    minTemp = c;
  }

  lcd.clear();

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

    bool alarm = (alarmOnHighTemp && c >= threshold && c < MAX_TEMP) || (!alarmOnHighTemp && c <= threshold);
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
      Serial.println();
      lcd.setCursor(0, 1);
      lcd.print(F("THRESHOLD  "));
      if (threshold >= 0 && threshold < 10.0) {
        lcd.print(F(" "));
      }
      lcd.print(tempToDisplay(threshold));
    }
  }
  
  if (!maxMinDisplay) { // normal mode
  
    if (c < MAX_TEMP) {
      Serial.print(F("Temperature: "));
      Serial.println(c);
      lcd.setCursor(0, 0);
      lcd.print(F("TEMPERATURE"));
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
