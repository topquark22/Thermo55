#include "Adafruit_MAX31855.h"
#include "LiquidCrystal_I2C.h"

#include "thermo55.h"

const int PIN_OUT = 2;
const int PIN_OUT_ = 3;

// SPI hardware configuration
const int thermoDO = 12; // MISO
const int thermoCS = 8; // Chip select
const int thermoCLK = 13; // SPI serial clock

Adafruit_MAX31855 thermocouple(thermoCLK, thermoCS, thermoDO);

// switch lcd display mode (normal or max/min)
const int PIN_BUTTON = 5;

// If wired to ground, alarm on low temp. Else alarm on high temp.
const int PIN_ALARM_DIR = 4;

bool alarmOnHighTemp;

// Connect LCD I2C pin SDA to A4
// Connect LCD I2C pin SCL to A5
LiquidCrystal_I2C lcd(LCD_I2C_ADDR, LCD_WIDTH, LCD_HEIGHT);

// analog input to set alarm threshold
const int PIN_THRESHOLD = A0;

 // highest reading for thermocouple
#define MAX_TEMP 1350
 // lowest reading for thermocouple
#define MIN_TEMP -200

//// track max and min temp since reset
float maxTemp = MIN_TEMP;
float minTemp = MAX_TEMP;
float maxTemps[MAXMIN_HOLD_CT];
float minTemps[MAXMIN_HOLD_CT];

// countdown time for backlight
int backlightCountdown;
 
int prevButton = HIGH;

int maxMinCtr = 0;

int sampleCt = 0;

float prevThreshold = -1;

float getThreshold() {
  int reading = analogRead(PIN_THRESHOLD);

  // interpolate
  float threshold = TEMP_LOW + (TEMP_HIGH - TEMP_LOW) * reading / 1023;

  if (abs(threshold - prevThreshold) > 0.25) {
    lcd.backlight();
    backlightCountdown = BACKLIGHT_TIME;
  }
  prevThreshold = threshold;
      
  return threshold;
}

const int BAUD_RATE = 9600;

bool floatEquals(float a, float b) {
  return abs(a - b) < 0.001;
}

void setOutput(bool value) {
  digitalWrite(PIN_OUT, value);
  digitalWrite(PIN_OUT_, !value);
}

void resetMaxMin() {
  maxTemp = MIN_TEMP;
  minTemp = MAX_TEMP;
  for (int i = 0; i < MAXMIN_HOLD_CT; i++) {
    maxTemps[i] = MIN_TEMP;
    minTemps[i] = MAX_TEMP;
  }
  sampleCt = 0;
}

void setup() {

  resetMaxMin();
  
  Serial.begin(BAUD_RATE);
  
  pinMode(PIN_OUT, OUTPUT);
  pinMode(PIN_OUT_, OUTPUT);
  pinMode(PIN_THRESHOLD, INPUT);
  pinMode(PIN_ALARM_DIR, INPUT_PULLUP);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
    
  setOutput(LOW);

  alarmOnHighTemp = digitalRead(PIN_ALARM_DIR);
  Serial.print(F("Will alarm on "));
  if (alarmOnHighTemp) {
    Serial.println(F("high temperature threshold"));
  } else {
    Serial.println(F("low temperature threshold"));
  }
  Serial.println();
  
  lcd.init();
  lcd.backlight();
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
    
  if (button) {
    backlightCountdown = BACKLIGHT_TIME;
    lcd.backlight();

  }

  if (backlightCountdown > 0) {
    backlightCountdown--;
    if (backlightCountdown == 0) {
      lcd.noBacklight();
      maxMinMode = false;
    }
  }
  
  lcd.clear();
  
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

  sampleCt++;

  if (c > maxTemp) {
    maxTemp = c;
  }
  if (c < minTemp) {
    minTemp = c;
  }
  
  // skip first few samples
  if (sampleCt >= MAXMIN_HOLD_CT) {
    maxTemps[maxMinCtr] = maxTemp;
    minTemps[maxMinCtr] = minTemp;
    maxMinCtr = (maxMinCtr + 1) % MAXMIN_HOLD_CT;
  }

  float threshold = getThreshold();

  setOutput((alarmOnHighTemp && c >= threshold) || (!alarmOnHighTemp && c <= threshold));

  
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

    float min = minTemps[maxMinCtr];
    lcd.setCursor(0, 0);
    lcd.print(F("MIN "));
    Serial.print(F("Minimum since last display: "));
    if (min + 1 > MAX_TEMP) {
      Serial.println(F("-"));
      lcd.print(F("-"));
    } else {
      Serial.println(min);
      lcd.print(min);
    }
    
    float max = maxTemps[maxMinCtr];
    lcd.setCursor(0, 1);
    lcd.print(F("MAX "));
    Serial.print(F("Maximum since last display: "));
    if (max - 1 < MIN_TEMP) {
      Serial.println(F("-"));
      lcd.print(F("-"));
    } else {
      Serial.println(max);
      lcd.print(max);
    }

    // hold display until button released
    while (!digitalRead(PIN_BUTTON)) {
      delay(100);
    }
    if (backlightCountdown <= 1) {
      resetMaxMin();
    }
  }

  delay(INTERVAL);
}
