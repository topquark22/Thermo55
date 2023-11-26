#include "Adafruit_MAX31855.h"
#include "LiquidCrystal_I2C.h"

 // highest reading for thermocouple
#define MAX_TEMP 1350
 // lowest reading for thermocouple
#define MIN_TEMP -200

// alarm threshold supported range in degrees C
// (Note: Type K thermocouple actually supports -200 to +1300)
const float TEMP_LOW = -40;
const float TEMP_HIGH = 110;

// LCD I2C address and size
const int LCD_I2C_ADDR = 0x27;
const int LCD_WIDTH = 16;
const int LCD_HEIGHT = 2;

const int PIN_OUT = 2;
const int PIN_OUT_ = 3;

// switch lcd display mode (normal or max/min)
const int PIN_BUTTON = 5;

// If wired to ground, alarm on low temp. Else alarm on high temp.
const int PIN_ALARM_DIR = 4;

// analog input to set alarm threshold
const int PIN_THRESHOLD = A0;

// SPI hardware configuration
const int thermoDO = 12; // MISO
const int thermoCS = 8; // Chip select
const int thermoCLK = 13; // SPI serial clock

Adafruit_MAX31855 thermocouple(thermoCLK, thermoCS, thermoDO);


bool alarmOnHighTemp;

// Connect LCD I2C pin SDA to A4
// Connect LCD I2C pin SCL to A5
LiquidCrystal_I2C lcd(LCD_I2C_ADDR, LCD_WIDTH, LCD_HEIGHT);

// track max and min temp since last measurement
float maxTemp = MIN_TEMP;
float minTemp = MAX_TEMP;

// time backlight stays on
const int  BACKLIGHT_TIME = 7;

// countdown time for backlight
int backlightCountdown;
 
// countdown time for max/min mode
int maxMinCountdown = 0;

int prevButton = HIGH;

// Number of passes through the loop. A sample is not taken every pass.
int loopCt = 0;

// Number of samples taken (loopCt / LOOPS_PER_SAMPLE)
int sampleCt = 0;

float prevThreshold = -1;

float getThreshold() {
  int reading = analogRead(PIN_THRESHOLD);

  // interpolate
  float threshold = TEMP_LOW + (TEMP_HIGH - TEMP_LOW) * reading / 1023;

  if (abs(threshold - prevThreshold) > 1.0) {
    lcd.backlight();
    backlightCountdown = BACKLIGHT_TIME;
  }
  prevThreshold = threshold;
      
  return threshold;
}

void setOutput(bool value) {
  digitalWrite(PIN_OUT, value);
  digitalWrite(PIN_OUT_, !value);
  if (value) {
    lcd.backlight();
    backlightCountdown = BACKLIGHT_TIME;
  }
}

const int BAUD_RATE = 9600;

void setup() {
  
  pinMode(PIN_OUT, OUTPUT);
  pinMode(PIN_OUT_, OUTPUT);
  pinMode(PIN_THRESHOLD, INPUT);
  pinMode(PIN_ALARM_DIR, INPUT_PULLUP);
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  Serial.begin(BAUD_RATE);

  setOutput(LOW);

  maxTemp = MIN_TEMP;
  minTemp = MAX_TEMP;

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
  maxMinCountdown = 0;

  // wait for MAX31855 to stabilize
  delay(500);
}

void loop() {

  bool button = !digitalRead(PIN_BUTTON);

  if (button && prevButton) {
    // button held down for 2 samples; switch to max/min
    maxMinCountdown = BACKLIGHT_TIME;
  }
  prevButton = button;
  
  lcd.clear();

  if (button) {
    backlightCountdown = BACKLIGHT_TIME;
    lcd.backlight();
  }

  if (backlightCountdown > 0) {
    backlightCountdown--;
    if (backlightCountdown == 0) {
      lcd.noBacklight();
    }
  }

  if (maxMinCountdown > 0) {
    maxMinCountdown--;
  }
  
  // Check for errors
  uint8_t error = thermocouple.readError();

  if (error) {
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

    while (true) {
      setOutput(HIGH);
      delay(200);
      setOutput(LOW);
      delay(200);
    }
  }

  // Read temperature in Celsius
  double c = thermocouple.readCelsius();

  if (c > maxTemp) {
    maxTemp = c;
  }
  if (c < minTemp) {
    minTemp = c;
  }

  float threshold = getThreshold();

  bool alarm = (alarmOnHighTemp && c >= threshold) || (!alarmOnHighTemp && c <= threshold);
  setOutput(alarm);

  if (alarm) {
    Serial.println(F("ALARM ON"));
  } else {
    Serial.println(F("ALARM OFF"));
  }

  if (maxMinCountdown == 0) { // normal mode
    
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

    lcd.setCursor(0, 0);
    lcd.print(F("MAX "));
    Serial.print(F("Maximum since last display: "));
    Serial.println(maxTemp);
    lcd.print(maxTemp);
    
    lcd.setCursor(0, 1);
    lcd.print(F("MIN "));
    Serial.print(F("Minimum since last display: "));
    Serial.println(minTemp);
    lcd.print(minTemp);
    
    Serial.println();

    // preserve display while button pressed
    while (!digitalRead(PIN_BUTTON)) {
      delay(100);
    }

    // reset MAX/MIN for next measurement
    if (maxMinCountdown <= 1) {
      maxTemp = MIN_TEMP;
      minTemp = MAX_TEMP;
    }
  }

  delay(1000);
}
