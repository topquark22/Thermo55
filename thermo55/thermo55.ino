#include "Adafruit_MAX31855.h"

const int PIN_LED = 2;

// SPI hardware configuration
const int thermoDO = 12; // MISO
const int thermoCS = 9; // Chip select
const int thermoCLK = 13; // SPI serial clock

Adafruit_MAX31855 thermocouple(thermoCLK, thermoCS, thermoDO);

const int BAUD_RATE = 9600;

float WARNING_THRESHOLD_C = 100;

void setup() {

  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED,LOW);
  
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
    Serial.print("Celsius: ");
    Serial.println(c);
    if (c >= WARNING_THRESHOLD_C) {
      digitalWrite(PIN_LED, HIGH);
    } else {
      digitalWrite(PIN_LED, LOW);
    }
  }

  // Delay between readings
  delay(1000);
}
