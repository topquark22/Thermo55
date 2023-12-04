#ifndef THERMO55_RADIO_H
#define THERMO55_RADIO_H

#include <RF24_config.h>
#include <nRF24L01.h>
#include <RF24.h>

// These wirings of CE, CSN are used for integrated Nano3/nRF24l01 boards
const int PIN_CE = 10;
const int PIN_CSN = 9;

// radio channel
const int PIN_CHANNEL = A0;

// radio power level jumpers
const uint8_t PIN_PWR2 = A2;
const uint8_t PIN_PWR1 = A3;

// radio channel
const int CHANNEL_BASE = 113;

const uint64_t DEVICE_ID = 0x7E0000A700LL;

void setupRadio(bool xmitMode);

float receiveCelsius();

void transmitCelsius(float c);

#endif
