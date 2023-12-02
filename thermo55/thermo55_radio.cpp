#include "thermo55.h"
#include "thermo55_radio.h"

byte commBuffer[4];

// match default SPI clock speed used by MAX31855
RF24 radio(PIN_CE, PIN_CSN, SPI_CLOCK_DIV4);

extern LiquidCrystal_I2C lcd;

void setupRadio(bool xmitMode) {

  // if wired low, use LOW power, else use MAX power
  pinMode(PIN_PWR, INPUT_PULLUP);
  rf24_pa_dbm_e power = digitalRead(PIN_PWR) ? RF24_PA_MAX : RF24_PA_LOW;

  pinMode(PIN_CHANNEL, INPUT_PULLUP);
  int channel = CHANNEL_BASE + 5 * digitalRead(PIN_CHANNEL);

  int deviceID = DEVICE_ID;

  Serial.println(F("Starting radio"));
  lcd.print(F("STARTING RADIO"));
  radio.begin();
  lcd.clear();

  if (!radio.isChipConnected()) {
    Serial.println(F("Radio not connected"));
    lcd.clear();
    lcd.print(F("NO RADIO"));
    delay(100);
    exit(1);
  }

  // Data rate: Can set to RF24_1MBPS, RF24_2MBPS, RF24_250KBPS (nRF24L01+ only)
  radio.setDataRate(RF24_1MBPS);
  radio.setPALevel(power);
  radio.setChannel(channel);

  lcd.clear();

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
  
  delay(2000);

  if (xmitMode) {
    radio.openWritingPipe(deviceID); // Get NRF24L01 ready to transmit
    radio.stopListening();
  } else { // recv mode
    radio.openReadingPipe(1, deviceID); // Get NRF24L01 ready to receive
    radio.startListening(); // Listen to see if information received
  }
}

float receiveCelsius() {
  while (!radio.available()) {
    delay(1000);
  }
  radio.read(commBuffer, 4); // Read data from the nRF24L01
  int value = (commBuffer[0] << 24) + (commBuffer[1] << 16) + (commBuffer[2] << 8) + commBuffer[3];
  float c = (float)value / 100;
  return c;
}

void transmitCelsius(float c) {
  int value = 100 * c;
  commBuffer[0] = (value >> 24) & 0xFF;
  commBuffer[1] = (value >> 16) & 0xFF;
  commBuffer[2] = (value >> 8) & 0xFF;
  commBuffer[3] = value & 0xFF;
  radio.write(commBuffer, 4);
}
