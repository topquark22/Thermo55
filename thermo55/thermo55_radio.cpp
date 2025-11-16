#include "thermo55.h"
#include "thermo55_radio.h"

uint8_t commBuffer[4]; // to allow for left-shift operations. Actual value is 8 bits

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

  // power 0=MIN, 1=LOW, 2=HIGH, 3=MAX
  rf24_pa_dbm_e power = (rf24_pa_dbm_e)(2 * digitalRead(PIN_PWR2_) + 1);
  Serial.print(F("Power set to ")); Serial.println(power);

  // Arduino Nano does not have enough pins to allocate one for this!
  //pinMode(PIN_CHANNEL, INPUT_PULLUP);
  int channel = CHANNEL_BASE; // + 5 * digitalRead(PIN_CHANNEL);

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

  uint64_t deviceID = DEVICE_ID;     // use full 40-bit/64-bit address

  // Both sides can TX and RX on the same address
  radio.openWritingPipe(deviceID);
  radio.openReadingPipe(1, deviceID);

  // Start in listening mode on both boards
  radio.startListening();
}

// assume radio enabled
float receiveCelsius() {
  if (!radio.available()) {
    return INF_TEMP;
  }
  radio.read(commBuffer, 4); // Read data from the nRF24L01
  uint32_t value = ( (uint32_t)commBuffer[0] << 24) |
                   ( (uint32_t)commBuffer[1] << 16) |
                   ( (uint32_t)commBuffer[2] << 8)  |
                     (uint32_t)commBuffer[3];
  float c = (float)value / 100;
  return c;
}

// assume radio enabled
void transmitCelsius(float c) {
  uint32_t value = 100 * c;
  commBuffer[0] = (value >> 24) & 0xFF;
  commBuffer[1] = (value >> 16) & 0xFF;
  commBuffer[2] = (value >> 8) & 0xFF;
  commBuffer[3] = value & 0xFF;
  radio.write(commBuffer, 4);
}

// Master side (LCD / receiver board):
// Send a 1-byte dummy request, then wait for a 4-byte temperature response.
bool requestCelsius(float *c) {
  if (!radioEnabled) {
    return false;
  }

  // 1-byte “please send me a temperature” request
  uint8_t reqByte = 0x55;

  // Send request
  radio.stopListening();
  bool ok = radio.write(&reqByte, 1);
  radio.startListening();

  if (!ok) {
    return false; // radio-level failure
  }

  // Wait up to ~100 ms for the response
  unsigned long start = millis();
  while (!radio.available()) {
    if (millis() - start > 100) {
      return false; // timeout
    }
  }

  // Read the 4-byte centi-degree response
  radio.read(commBuffer, 4);
  uint32_t value = ( (uint32_t)commBuffer[0] << 24) |
              ( (uint32_t)commBuffer[1] << 16) |
              ( (uint32_t)commBuffer[2] << 8)  |
                (uint32_t)commBuffer[3];

  *c = (float)value / 100.0f;
  return true;
}

// Slave side (thermocouple / transmitter board):
// If a request packet is waiting, consume it and immediately send
// the current temperature as a 4-byte centi-degree payload.
void serviceTemperatureRequests(float tempC) {
  if (!radioEnabled) {
    return;
  }

  if (!radio.available()) {
    return; // nothing to do
  }

  // Consume the request byte(s); we don't really care about contents.
  uint8_t reqBuf[8];
  while (radio.available()) {
    radio.read(reqBuf, sizeof(reqBuf));
  }

  // Prepare the reply payload
 uint32_t value = (uint32_t)(tempC * 100.0f);
  commBuffer[0] = (value >> 24) & 0xFF;
  commBuffer[1] = (value >> 16) & 0xFF;
  commBuffer[2] = (value >> 8)  & 0xFF;
  commBuffer[3] = value & 0xFF;

  radio.stopListening();
  radio.write(commBuffer, 4);
  radio.startListening();
}


