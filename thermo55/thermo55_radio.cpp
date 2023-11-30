#include <RF24_config.h>
#include <nRF24L01.h>
#include <RF24.h>

#include "thermo55.h"
#include "thermo55_radio.h"

extern bool xmitMode;

byte commBuffer[4];

RF24 radio(PIN_CE, PIN_CSN);

void setupRadio() {

  // power 0=MIN, 1=LOW, 2=HIGH, 3=MAX;
  // if wired low, use LOW power, else use MAX power
  pinMode(PIN_PWR, INPUT_PULLUP);
  int power = digitalRead(PIN_PWR) ? 3 : 1;

  pinMode(PIN_CHANNEL, INPUT_PULLUP);
  int channel = CHANNEL_BASE + 5 * digitalRead(PIN_CHANNEL);

  int deviceID = DEVICE_ID;

  radio.begin();

  if (!radio.isChipConnected()) {
    Serial.println(F("Radio not connected"));
    blinkLED(100);
    } else {
    Serial.print(F("Radio power set to "));
    Serial.println(channel);
    Serial.print(F("Radio channel set to "));
    Serial.println(channel);

    // Data rate: Can set to RF24_1MBPS, RF24_2MBPS, RF24_250KBPS (nRF24L01+ only)
    radio.setDataRate(RF24_1MBPS);
    radio.setPALevel(power);
    radio.setChannel(channel);

    if (xmitMode) {
      radio.openWritingPipe(deviceID); // Get NRF24L01 ready to transmit
      radio.stopListening();
    } else { // recv mode
      radio.openReadingPipe(1, deviceID); // Get NRF24L01 ready to receive
      radio.startListening(); // Listen to see if information received
    }
  }
}

float receiveCelsius() {
  while (!radio.available()) {
    delay(100);
  }
  radio.read(commBuffer, 4); // Read data from the nRF24L01
  int value = (commBuffer[0] << 24) + (commBuffer[1] << 16) + (commBuffer[2] << 8) + commBuffer[3];
  float c = (float)value / 100;
  return c;
}

void transmitCelsius(float c) {
  int value = 100 * c;
  commBuffer[0] = (value >> 24) & 0xFF;
  commBuffer[1] = (value >> 16) & 0xFF;
  commBuffer[2] = (value >> 8) & 0xFF;
  commBuffer[3] = value & 0xFF;
  radio.write(commBuffer, 4);
}