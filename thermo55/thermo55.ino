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

// Delay between readings
const int INTERVAL = 1000;

const int BAUD_RATE = 9600;

void setup() {

  pinMode(PIN_OUT, OUTPUT);
  pinMode(PIN_OUT_, OUTPUT);
  digitalWrite(PIN_OUT,LOW);
  digitalWrite(PIN_OUT_, HIGH);
  pinMode(PIN_THRESHOLD, INPUT);

  lcd.begin(16,2);
  
  Serial.begin(BAUD_RATE);

  // wait for MAX31855 to stabilize
  delay(500);
}

void loop() {
  // Read temperature in Celsius
  double c = thermocouple.readCelsius();
  // Check for errors
  uint8_t error = thermocouple.readError();

  if (error) {
    
    digitalWrite(PIN_OUT, LOW);
    digitalWrite(PIN_OUT_, HIGH);
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Error:");
    lcd.setCursor(0, 1);
    
    Serial.print("Error: ");
    if (error & MAX31855_FAULT_OPEN) {
      Serial.println("Open Circuit!");
      lcd.print("Open circuit");
    }
    if (error & MAX31855_FAULT_SHORT_GND) {
      Serial.println("Short to GND!");
      lcd.print("Short to GND");
    }
    if (error & MAX31855_FAULT_SHORT_VCC) {
      Serial.println("Short to VCC!");
      lcd.print("Short to VCC)");
    }
    delay(100); // flush serial buffer
    exit(1);
  }
  
  float threshold = alarmTemperature();

  Serial.print("Temp:  ");
  Serial.println(c);
  Serial.print("Alarm: ");
  Serial.println(threshold);
  Serial.println();
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp:  ");
  lcd.print(c);
  lcd.setCursor(0, 1);
  lcd.print("Alarm: ");
  lcd.print(threshold);

  if (c >= threshold) {
    digitalWrite(PIN_OUT, HIGH);
    digitalWrite(PIN_OUT_, LOW);
  } else {
    digitalWrite(PIN_OUT, LOW);
    digitalWrite(PIN_OUT_, HIGH);
  }

  delay(INTERVAL);
}
