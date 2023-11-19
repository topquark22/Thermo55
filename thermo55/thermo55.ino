#include "Adafruit_MAX31855.h"
#include "LiquidCrystal.h"

const int PIN_OUT = 2;
const int PIN_OUT_ = 3;

// SPI hardware configuration
const int thermoDO = 12; // MISO
const int thermoCS = A5; // Chip select
const int thermoCLK = 13; // SPI serial clock

Adafruit_MAX31855 thermocouple(thermoCLK, thermoCS, thermoDO);

// LCD1602 pins
const int PIN_RS = 7;
const int PIN_E = 6;
const int PIN_DS4 = A1;
const int PIN_DS3 = A2;
const int PIN_DS2 = A3;
const int PIN_DS1 = A4;

LiquidCrystal lcd(PIN_RS, PIN_E, PIN_DS4, PIN_DS3, PIN_DS2, PIN_DS1);

const int PIN_THRESHOLD = A0;

// alarm threshold range in degrees C
const float TEMP_LOW = -20;
const float TEMP_HIGH = 200;

float alarmTemperature() {
  int reading = analogRead(PIN_THRESHOLD);
  // interpolate
  float alarmTemp = TEMP_LOW + (TEMP_HIGH - TEMP_LOW) * reading / 1023;
  return alarmTemp;
}

const int BAUD_RATE = 9600;

void setup() {

  pinMode(PIN_OUT, OUTPUT);
  pinMode(PIN_OUT_, OUTPUT);
  digitalWrite(PIN_OUT,LOW);
  digitalWrite(PIN_OUT_, HIGH);
  pinMode(PIN_THRESHOLD, INPUT);
  
  Serial.begin(BAUD_RATE);
  Serial.println("MAX31855 test");
  // wait for MAX31855 to stabilize
  delay(500);
}

void loop() {
  // Read temperature in Celsius
  double c = thermocouple.readCelsius();
  // Check for errors
  uint8_t error = thermocouple.readError();

  if (error) {
    Serial.print("Error: ");
    if (error & MAX31855_FAULT_OPEN) {
      Serial.println("Open Circuit!");
    }
    if (error & MAX31855_FAULT_SHORT_GND) {
      Serial.println("Short to GND!");
    }
    if (error & MAX31855_FAULT_SHORT_VCC) {
      Serial.println("Short to VCC!");
    }
  } else {
    float threshold = alarmTemperature();
    Serial.print("Alarm threshold: ");
    Serial.println(threshold);
    Serial.print("Celsius: ");
    Serial.println(c);
    if (c >= threshold) {
      digitalWrite(PIN_OUT, HIGH);
      digitalWrite(PIN_OUT_, LOW);
    } else {
      digitalWrite(PIN_OUT, LOW);
      digitalWrite(PIN_OUT_, HIGH);
    }
  }

  // Delay between readings
  delay(1000);
}
