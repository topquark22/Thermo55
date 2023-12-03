#ifndef THERMO55_H
#define THERMO55_H

#include <Arduino.h>
#include "LiquidCrystal_I2C.h"

const int PIN_OUT = 2;
const int PIN_OUT_ = 3;

void setOutput(bool value);

void blinkLED(int millis); 

#endif
