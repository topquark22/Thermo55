#include "Adafruit_MAX31855.h"

// SPI hardware configuration
int thermoDO = 12; // MISO
int thermoCS = 10; // Chip select
int thermoCLK = 13; // SPI serial clock

Adafruit_MAX31855 thermocouple(thermoCLK, thermoCS, thermoDO);

void setup() {
  Serial.begin(9600);
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
  }
  // Delay between readings
  delay(1000);
}
