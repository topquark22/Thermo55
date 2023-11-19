#include "Adafruit_MAX31855.h"

const int PIN_OUT = 2;
const int PIN_OUT_ = 3;

// SPI hardware configuration
const int thermoDO = 12; // MISO
const int thermoCS = A5; // Chip select
const int thermoCLK = 13; // SPI serial clock

Adafruit_MAX31855 thermocouple(thermoCLK, thermoCS, thermoDO);

const int BAUD_RATE = 9600;

float WARNING_THRESHOLD_C = 24; // TODO read this from nRLF24L radio

void setup() {

  pinMode(PIN_OUT, OUTPUT);
  pinMode(PIN_OUT_, OUTPUT);
  digitalWrite(PIN_OUT,LOW);
  digitalWrite(PIN_OUT_, HIGH);
  
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
