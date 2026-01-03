#pragma once

#include <Arduino.h>

#if NUM_ANALOG_INPUTS < 8
  #error Board does not support analog inputs on pins A6, A7
#endif

#include "LiquidCrystal_I2C.h"

 // above highest reading for K-type thermocouple
#define MAX_TEMP 99999
 // below lowest reading for K-type thermocouple
#define MIN_TEMP -99999

void errExit();