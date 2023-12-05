#include "thermo55.h"
#include "thermo55_radio.h"

byte commBuffer[4];

RF24 radio(PIN_CE, PIN_CSN, SPI_CLOCK_DIV4);

extern LiquidCrystal_I2C lcd;

bool radioEnabled;

bool isRadioEnabled() {
  return radioEnabled;
}

void setupRadio(bool xmitMode) {

  lcd.clear();

  pinMode(PIN_ENABLE_RADIO, INPUT_PULLUP);
  radioEnabled = !digitalRead(PIN_ENABLE_RADIO);

  if (!xmitMode && !radioEnabled) {
    Serial.println(F("Unsupported configuration"));
    lcd.print("UNSUPPORTED");
    lcd.setCursor(0, 1);
    lcd.print(F("CONFIGURATION"));
    delay(100);
    exit(1);
  }

  if (!radioEnabled) {
    return;
  }

// power level jumpers
  pinMode(PIN_PWR2_, INPUT_PULLUP);
  pinMode(PIN_PWR1_, INPUT_PULLUP);
  // power 0=MIN, 1=LOW, 2=HIGH, 3=MAX
  rf24_pa_dbm_e power = 2 * digitalRead(PIN_PWR2_) + digitalRead(PIN_PWR1_);
  Serial.print(F("Power set to ")); Serial.println(power);

  // Arduino Nano does not have enough pins to allocate one for this!
  //pinMode(PIN_CHANNEL, INPUT_PULLUP);
  int channel = CHANNEL_BASE; // + 5 * digitalRead(PIN_CHANNEL);

  int deviceID = DEVICE_ID;

  radio.begin();  

  if (radio.isChipConnected()) {
    // Data rate: Can set to RF24_1MBPS, RF24_2MBPS, RF24_250KBPS (nRF24L01+ only)
    radio.setDataRate(RF24_1MBPS);
    radio.setPALevel(power);
    radio.setChannel(channel);

    Serial.print(F("Radio power set to "));
    Serial.println(power);
    lcd.print(F("POWER"));
    lcd.setCursor(8, 0);
    lcd.print(power);

    Serial.print(F("Radio channel set to "));
    Serial.println(channel);
    lcd.setCursor(0, 1);
    lcd.print(F("CHANNEL"));
    lcd.setCursor(8, 1);
    lcd.print(channel);
    delay(1000);
  } else {
    Serial.println(F("Radio fault"));
    lcd.clear();
    lcd.print(F("RADIO FAULT"));
    delay(100);
    exit(1);
  }

  if (xmitMode) {
    radio.openWritingPipe(deviceID); // Get NRF24L01 ready to transmit
    radio.stopListening();
  } else { // recv mode
    radio.openReadingPipe(1, deviceID); // Get NRF24L01 ready to receive
    radio.startListening(); // Listen to see if information received
  }
}

// assume radio enabled
float receiveCelsius() {
  if (!radio.available()) {
    return INFINITY;
  }
  radio.read(commBuffer, 4); // Read data from the nRF24L01
  int value = (commBuffer[0] << 24) + (commBuffer[1] << 16) + (commBuffer[2] << 8) + commBuffer[3];
  float c = (float)value / 100;
  return c;
}

// assume radio enabled
void transmitCelsius(float c) {
  int value = 100 * c;
  commBuffer[0] = (value >> 24) & 0xFF;
  commBuffer[1] = (value >> 16) & 0xFF;
  commBuffer[2] = (value >> 8) & 0xFF;
  commBuffer[3] = value & 0xFF;
  radio.write(commBuffer, 4);
}
