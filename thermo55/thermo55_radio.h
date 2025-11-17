#ifndef THERMO55_RADIO_H
#define THERMO55_RADIO_H

#include <RF24_config.h>
#include <nRF24L01.h>
#include <RF24.h>

#include "thermo55.h"

// These wirings of CE, CSN are used for integrated Nano3/nRF24L01 boards
const uint8_t PIN_CE  = 10;
const uint8_t PIN_CSN = 9;

// Jumper to enable / disable radio entirely (A0 = enabled when tied low)
const uint8_t PIN_ENABLE_RADIO = A0;

// Radio power level jumper (A2: LOW/HIGH)
const uint8_t PIN_PWR2_ = A2;

// Radio channel (allow override at compile time)
#ifndef THERMO55_RADIO_CHANNEL
  #define THERMO55_RADIO_CHANNEL 113
#endif
const uint8_t CHANNEL_BASE = THERMO55_RADIO_CHANNEL;

// Unique 5-byte address used by both nodes (allow override at compile time)
#ifndef THERMO55_DEVICE_ID
  #define THERMO55_DEVICE_ID 0x7E0000A700LL
#endif
const uint64_t DEVICE_ID = THERMO55_DEVICE_ID;

// Initialise the radio; xmitMode is true on the thermocouple node, false on the display node.
void setupRadio(bool xmitMode);

// Returns whether the radio is enabled via the A0 jumper and successfully initialised.
bool isRadioEnabled();

// Robust request/reply API using ACK payloads and sequence numbers.
// On the display (receiver) node:
bool requestCelsius(float &outC);

// On the thermocouple (transmitter) node:
void processRadioAsSlave(float currentTemp);

#endif
