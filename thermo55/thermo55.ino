#include "Adafruit_MAX31855.h"
#include "LiquidCrystal.h"

#include "thermo55.h"

const int PIN_OUT = 2;
const int PIN_OUT_ = 3;

// SPI hardware configuration
const int thermoDO = 12; // MISO
const int thermoCS = 8; // Chip select
const int thermoCLK = 13; // SPI serial clock

Adafruit_MAX31855 thermocouple(thermoCLK, thermoCS, thermoDO);

// LCD1602 pins for parallel interface
const int PIN_RS = 6;
const int PIN_E = 7;
const int PIN_DS4 = A5; // orange
const int PIN_DS5 = A2; // blue
const int PIN_DS6 = A3; // brown
const int PIN_DS7 = A4; // yellow

// switch lcd display mode (normal or max/min)
const int PIN_BUTTON = 5;

// If wired to ground, alarm on low temp. Else alarm on high temp.
const int PIN_ALARM_DIR = 4;

bool alarmOnHighTemp;

LiquidCrystal lcd(PIN_RS, PIN_E, PIN_DS4, PIN_DS5, PIN_DS6, PIN_DS7);

// analog input to set alarm threshold
const int PIN_THRESHOLD = A0;

 // highest reading for thermocouple
#define MAX_TEMP 1350
 // lowest reading for thermocouple
#define MIN_TEMP -200

//// track max and min temp since reset
float maxTemp = MIN_TEMP;
float minTemp = MAX_TEMP;
float maxTemps[LAG_TIME];
float minTemps[LAG_TIME];

// countdown time for backlight
int backlightCountdown;
 
int prevButton = HIGH;

int maxMinCtr = 0;

// Number of passes through the loop. A sample is not taken every pass.
int loopCt = 0;

// Number of samples taken (loopCt / LOOPS_PER_SAMPLE)
int sampleCt = 0;

float prevThreshold = -1;

float getThreshold() {
  int reading = analogRead(PIN_THRESHOLD);

  // interpolate
  float threshold = TEMP_LOW + (TEMP_HIGH - TEMP_LOW) * reading / 1023;

  if (abs(threshold - prevThreshold) > 0.25) {
    lcd.display();
    backlightCountdown = BACKLIGHT_TIME;
  }
  prevThreshold = threshold;
      
  return threshold;
}

const int BAUD_RATE = 9600;

void setOutput(bool value) {
  digitalWrite(PIN_OUT, value);
  digitalWrite(PIN_OUT_, !value);
}

void resetMaxMin() {
  maxTemp = MIN_TEMP;
  minTemp = MAX_TEMP;
  for (int i = 0; i < LAG_TIME; i++) {
    maxTemps[i] = MIN_TEMP;
    minTemps[i] = MAX_TEMP;
  }
  sampleCt = 0;
}

int leadTime;

const int lagTime = LAG_TIME;

void setup() {
  
  pinMode(PIN_OUT, OUTPUT);
  pinMode(PIN_OUT_, OUTPUT);
  pinMode(PIN_THRESHOLD, INPUT);
  pinMode(PIN_ALARM_DIR, INPUT_PULLUP);
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  // parallel interface has no pins available to set this with jumpers, so it is unsupported
  leadTime = 1;

  setOutput(LOW);

  Serial.begin(BAUD_RATE);

  resetMaxMin();

  alarmOnHighTemp = digitalRead(PIN_ALARM_DIR);
  Serial.print(F("Will alarm on "));
  if (alarmOnHighTemp) {
    Serial.println(F("high temperature threshold"));
  } else {
    Serial.println(F("low temperature threshold"));
  }
  Serial.println();
  
  lcd.begin(LCD_WIDTH, LCD_HEIGHT);
  lcd.display();
  
  backlightCountdown = BACKLIGHT_TIME;

  // wait for MAX31855 to stabilize
  delay(500);
}

bool maxMinMode = false;

void loop() {

  bool button = !digitalRead(PIN_BUTTON);

  if ((button && prevButton) || (backlightCountdown > 0 && maxMinMode)) {
    // button held down for 2 samples; switch to max/min
    maxMinMode = true;
  }
  
  prevButton = button;
  
  lcd.clear();

  if (button) {
    backlightCountdown = BACKLIGHT_TIME;
    lcd.display();
  }

  if (backlightCountdown > 0) {
    backlightCountdown--;
    if (backlightCountdown == 0) {
      lcd.noDisplay();
      maxMinMode = false;
    }
  }
  
  // Read temperature in Celsius
  double c = thermocouple.readCelsius();
  // Check for errors
  uint8_t error = thermocouple.readError();

  if (error) {
    
    setOutput(LOW);
    
    lcd.print(F("ERROR"));
    lcd.setCursor(0, 1);
    
    Serial.print("Error: ");
    if (error & MAX31855_FAULT_OPEN) {
      Serial.println(F("Open Circuit!"));
      lcd.print(F("OPEN CIRCUIT"));
    }
    if (error & MAX31855_FAULT_SHORT_GND) {
      Serial.println(F("Short to GND!"));
      lcd.print(F("SHORT TO GND"));
    }
    if (error & MAX31855_FAULT_SHORT_VCC) {
      Serial.println(F("Short to VCC!"));
      lcd.print(F("SHORT TO VCC"));
    }
    delay(100); // flush serial buffer
    exit(1);
  }

  if (loopCt >= leadTime) { // skip first few samples
      if (c > maxTemp) {
        maxTemp = c;
      }
      if (c < minTemp) {
        minTemp = c;
      }
  }

  maxTemps[loopCt % lagTime] = maxTemp;
  minTemps[loopCt % lagTime] = minTemp;
  loopCt++;

  float threshold = getThreshold();

  bool alarm = (alarmOnHighTemp && c >= threshold) || (!alarmOnHighTemp && c <= threshold);
  setOutput(alarm);

  if (alarm) {
    Serial.println(F("ALARM ON"));
  } else {
    Serial.println(F("ALARM OFF"));
  }

  if (!maxMinMode) { // normal mode
    
    Serial.print(F("Temperature: "));
    Serial.println(c);
    Serial.print(F("Threshold:   "));
    Serial.println(threshold);
    Serial.println();
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("TEMPERATURE"));
    lcd.print(c);
    lcd.setCursor(0, 1);
    lcd.print(F("THRESHOLD  "));
    lcd.print(threshold);
    
  } else { // Max/Min mode

    float max = maxTemps[loopCt % lagTime];
    float min = minTemps[loopCt % lagTime];

    bool maxMinUndefined = (max - 1 < MIN_TEMP);

    lcd.setCursor(0, 0);
    lcd.print(F("MAX "));
    Serial.print(F("Maximum since last display: "));
    if (maxMinUndefined) {
      Serial.println(F("-"));
      lcd.print(F("-"));
    } else {
      Serial.println(max);
      lcd.print(max);
    }
    
    lcd.setCursor(0, 1);
    lcd.print(F("MIN "));
    Serial.print(F("Minimum since last display: "));
    if (maxMinUndefined) {
      Serial.println(F("-"));
      lcd.print(F("-"));
    } else {
      Serial.println(min);
      lcd.print(min);
    }
    
    Serial.println();

    // preserve display while button pressed
    while (!digitalRead(PIN_BUTTON)) {
      delay(100);
    }

    maxTemp = MIN_TEMP;
    minTemp = MAX_TEMP;
  }

  delay(INTERVAL);
}
