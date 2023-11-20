#include "Adafruit_MAX31855.h"
#include "LiquidCrystal.h"

const int PIN_OUT = 2;
const int PIN_OUT_ = 3;

// SPI hardware configuration
const int thermoDO = 12; // MISO
const int thermoCS = 8; // Chip select
const int thermoCLK = 13; // SPI serial clock

Adafruit_MAX31855 thermocouple(thermoCLK, thermoCS, thermoDO);

// LCD1602 pins
const int PIN_RS = 7; // supposedly MISO ?
const int PIN_E = 6; // supposedly MOSI ?
const int PIN_DS4 = A1;
const int PIN_DS3 = A2;
const int PIN_DS2 = A3;
const int PIN_DS1 = A4;

// for future use, switch lcd display mode (for max/min etc.)
const int PIN_DISP_SEL = A5;

// Alarm direction: If wired to ground, alarm on low temp. Else alarm on high temp.
const int PIN_ALARM_DIR = 4;

bool highAlarm;

LiquidCrystal lcd(PIN_RS, PIN_E, PIN_DS4, PIN_DS3, PIN_DS2, PIN_DS1);

// analog input to set alarm threshold
const int PIN_THRESHOLD = A0;

// alarm threshold supported range in degrees C
// (Note: Type K thermocouple actually supports -200 to +1300)
const float TEMP_LOW = -20;
const float TEMP_HIGH = 200;

// track max and min temp since reset
float maxTemp = -200; // lowest reading for thermocouple
float minTemp = 1325; // highest reading for thermocouple

float alarmTemperature() {
  int reading = analogRead(PIN_THRESHOLD);
  // interpolate
  float alarmTemp = TEMP_LOW + (TEMP_HIGH - TEMP_LOW) * reading / 1023;
  return alarmTemp;
}

// Delay between readings
const int INTERVAL = 1000;

const int BAUD_RATE = 9600;

void setOutput(bool value) {
  digitalWrite(PIN_OUT, value);
  digitalWrite(PIN_OUT_, !value);
}
void setup() {

  Serial.begin(BAUD_RATE);
  
  pinMode(PIN_OUT, OUTPUT);
  pinMode(PIN_OUT_, OUTPUT);
  pinMode(PIN_THRESHOLD, INPUT);
  pinMode(PIN_ALARM_DIR, INPUT_PULLUP);
  pinMode(PIN_DISP_SEL, INPUT_PULLUP);
    
  setOutput(LOW);

  highAlarm = digitalRead(PIN_ALARM_DIR);
  Serial.print(F("Will alarm on "));
  if (highAlarm) {
    Serial.println(F("high temperature threshold"));
  } else {
    Serial.println(F("low temperature threshold"));
  }
  Serial.println();
  
  lcd.begin(16, 2);

  // wait for MAX31855 to stabilize
  delay(500);
}

void loop() {
  // Read temperature in Celsius
  double c = thermocouple.readCelsius();
  // Check for errors
  uint8_t error = thermocouple.readError();

  if (error) {
    
    setOutput(LOW);
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Error:");
    lcd.setCursor(0, 1);
    
    Serial.print("Error: ");
    if (error & MAX31855_FAULT_OPEN) {
      Serial.println(F("Open Circuit!"));
      lcd.print("Open circuit");
    }
    if (error & MAX31855_FAULT_SHORT_GND) {
      Serial.println(F("Short to GND!"));
      lcd.print("Short to GND");
    }
    if (error & MAX31855_FAULT_SHORT_VCC) {
      Serial.println(F("Short to VCC!"));
      lcd.print("Short to VCC)");
    }
    delay(100); // flush serial buffer
    exit(1);
  }

  if (c > maxTemp) {
    maxTemp = c;
  }
  if (c < minTemp) {
    minTemp = c;
  }
  
  float threshold = alarmTemperature();

  Serial.print(F("Temp:  "));
  Serial.println(c);
  Serial.print(F("Alarm: "));
  Serial.println(threshold);
  Serial.println();
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp:  ");
  lcd.print(c);
  lcd.setCursor(0, 1);
  lcd.print("Alarm: ");
  lcd.print(threshold);

  if ((highAlarm && c >= threshold) || (!highAlarm && c <= threshold)) {
    setOutput(HIGH);
  } else {
    setOutput(LOW);
  }

  delay(INTERVAL);
}
