#include <RF24_config.h>
#include <nRF24L01.h>
#include <RF24.h>

#include "thermo55.h"
#include "thermo55_radio.h"

extern bool xmitMode;

RF24 radio(PIN_CE, PIN_CSN);

void setupRadio() {

  // power 0=MIN, 1=LOW, 2=HIGH, 3=MAX;
  // if wired low, use LOW power, else use MAX power
  pinMode(PIN_PWR, INPUT_PULLUP);
  int power = digitalRead(PIN_PWR) ? 3 : 1;

  int channel = CHANNEL_BASE + 5 * digitalRead(PIN_CHANNEL);

  int deviceID = DEVICE_ID;

  if (!radio.isChipConnected()) {
    Serial.println(F("Radio not connected"));
    blinkLED(100);
    } else {
    Serial.print(F("Radio power set to "));
    Serial.println(channel);
    Serial.print(F("Radio channel set to "));
    Serial.println(channel);

    radio.begin();

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
  // TODO
	delay(1000);
	return 0.0;
}

void transmitCelsius(float c) {
	// TODO
}