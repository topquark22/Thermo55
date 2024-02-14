#ifndef THERMO55_H
#define THERMO55_H

#include <Arduino.h>
#include "LiquidCrystal_I2C.h"

// alert output
const int PIN_OUT = 2;

// auxiliary output control
const int PIN_AUX_ENABLE = 3;

void setOutput(bool value);

// Enable output to any auxiliary circuit. Generally we want to do this until the first time the threshold is triggered.
void enableAuxOutput(bool value);

// read akways-on display switch
bool isAlwaysOnDisplay();

void blinkLED(int millis); 

#endif
