#ifndef THERMO55_H
#define THERMO55_H

#include <Arduino.h>
#include "LiquidCrystal_I2C.h"



void setOutput(bool value);

// read akways-on display switch
bool isAlwaysOnDisplay();

void blinkLED(int millis); 

#endif
