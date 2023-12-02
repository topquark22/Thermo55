#ifndef THERMO55_H
#define THERMO55_H

#include <Arduino.h>
#include <SPI.h>
#include "LiquidCrystal_I2C.h"

// above highest reading for K-type thermocouple
#define INFINITY 99999
// below lowest reading for K-type thermocouple
#define NEGATIVE_INFINITY -99999

const int PIN_OUT = 2;
const int PIN_OUT_ = 3;

void setOutput(bool value);

void blinkLED(int millis); 

#endif
