#ifndef THERMO55_RADIO_H
#define THERMO55_RADIO_H

#include <RF24_config.h>
#include <nRF24L01.h>
#include <RF24.h>

// These wirings of CE, CSN are used for integrated Nano3/nRF24l01 boards
const PROGMEM uint8_t PIN_CE = 10;
const PROGMEM uint8_t PIN_CSN = 9;

const PROGMEM uint8_t PIN_ENABLE_RADIO = A0;

// radio power level jumpers
const PROGMEM uint8_t PIN_PWR2_ = A2;

// radio channel (allow to override at compile time)
#ifndef THERMO55_RADIO_CHANNEL
  #define THERMO55_RADIO_CHANNEL 113
#endif
const int CHANNEL_BASE = THERMO55_RADIO_CHANNEL;

// 40-bit radio device ID (allow to override at compile time)
#ifndef THERMO55_DEVICE_ID
  #define THERMO55_DEVICE_ID 0x7E0000A700LL
#endif

const uint64_t DEVICE_ID = THERMO55_DEVICE_ID;

void setupRadio(bool xmitMode);

boolean isRadioEnabled();

// original one-way helpers (still available if you want them)
float receiveCelsius();
void transmitCelsius(float c);

// NEW: request/response API
bool requestCelsius(float *c);
void serviceTemperatureRequests(float tempC);

#endif
