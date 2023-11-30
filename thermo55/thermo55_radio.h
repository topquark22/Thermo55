#ifndef R
#define R

//#include <Arduino.h>

// These wirings of CE, CSN are used for integrated Nano3/nRF24l01 boards
const int PIN_CE = 10;
const int PIN_CSN = 9;

const uint8_t PIN_PWR = A2;
const int PIN_CHANNEL = A3;

const int CHANNEL_BASE = 113;

const uint64_t DEVICE_ID = 0x7E000E7E00;

void setupRadio();

float receiveCelsius();

void transmitCelsius(float c);


#endif