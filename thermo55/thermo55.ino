#include "Adafruit_MAX31855.h"
#include "LiquidCrystal_I2C.h"

 // above highest reading for K-type thermocouple
#define POSITIVE_INFINITY 1351
 // below lowest reading for K-type thermocouple
#define NEGATIVE_INFINITY -201

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
float maxTemp = NEGATIVE_INFINITY;
float minTemp = POSITIVE_INFINITY;

// time display stays on
const int  DISPLAY_TIME = 10;

// countdown time for display
int displayCountdown;
 
// display max/min mode
bool maxMinDisplay;

int prevButton = HIGH;

float prevThreshold = NEGATIVE_INFINITY;

float getThreshold() {
  int reading = analogRead(PIN_THRESHOLD);

  // interpolate
  float threshold = TEMP_LOW + (TEMP_HIGH - TEMP_LOW) * reading / 1023;

  if (abs(threshold - prevThreshold) > 0.5) {
    displayCountdown = DISPLAY_TIME;
  }
  prevThreshold = threshold;
      
  return threshold;
}

void setOutput(bool value) {
  digitalWrite(PIN_OUT, value);
  digitalWrite(PIN_OUT_, !value);
  if (value) {
    displayCountdown = DISPLAY_TIME;
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

  maxTemp = NEGATIVE_INFINITY;
  minTemp = POSITIVE_INFINITY;

  alarmOnHighTemp = digitalRead(PIN_ALARM_DIR);
  Serial.print(F("Will alarm on "));
  if (alarmOnHighTemp) {
    Serial.println(F("high temperature threshold"));
  } else {
    Serial.println(F("low temperature threshold"));
  }
  Serial.println();
  
  lcd.init();
  
  displayCountdown = DISPLAY_TIME;
  maxMinDisplay = false;

  // wait for MAX31855 to stabilize
  delay(500);
}

void checkErrors() {
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
}
void loop() {

  lcd.clear();

  checkErrors();

  bool button = !digitalRead(PIN_BUTTON);

   if (button) {
    displayCountdown = DISPLAY_TIME;
    if (maxMinDisplay) {
      // button pressed during max/min display; reset values
      maxTemp = NEGATIVE_INFINITY;
      minTemp = POSITIVE_INFINITY;
    }
  }
  if (button && prevButton) {
    // button held down for 2 samples; switch to max/min
    maxMinDisplay = true;
  }
  prevButton = button;

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

  if (displayCountdown > 0) {
    displayCountdown--;
      lcd.display();
      lcd.backlight();
  } else {
      lcd.noDisplay();
      lcd.noBacklight();
      maxMinMode = false;
  }

  if (!maxMinDisplay) { // normal mode
    
    Serial.print(F("Temperature: "));
    Serial.println(c);
    Serial.print(F("Threshold:   "));
    Serial.println(threshold);
    Serial.println();
    
    lcd.setCursor(0, 0);
    lcd.print(F("TEMPERATURE"));
    if (c >= 0 && c < 10.0) {
      lcd.print(F(" "));
    }
    lcd.print(c);
    lcd.setCursor(0, 1);
    lcd.print(F("THRESHOLD  "));
    if (threshold >= 0 && threshold < 10.0) {
      lcd.print(F(" "));
    }
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
  }

  delay(1000);
}
