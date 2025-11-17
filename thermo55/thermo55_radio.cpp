#include "thermo55.h"
#include "thermo55_radio.h"

#include <SPI.h>

RF24 radio(PIN_CE, PIN_CSN, SPI_CLOCK_DIV4);

extern LiquidCrystal_I2C lcd;

static bool radioEnabled = false;
static bool isMasterNode = false;   // false on thermocouple node (xmitMode==true), true on display node

bool isRadioEnabled() {
  return radioEnabled;
}

// Simple request / response packet formats.
// All multi-byte fields are sent MSB-first (big-endian) over the air.
struct ThermoReq {
  uint8_t  cmd;      // 1 = READ_TEMP
  uint8_t  reserved; // currently unused
  uint16_t seq;      // sequence number from master
};

struct ThermoResp {
  uint8_t  status;   // 0 = OK, non-zero = error
  uint8_t  reserved; // currently unused
  uint16_t seq;      // echoed sequence number
  int32_t  tempC_x100; // temperature in centi-degrees Celsius
};

static uint16_t g_seqCounter = 0;

void setupRadio(bool xmitMode) {
  lcd.clear();

  pinMode(PIN_ENABLE_RADIO, INPUT_PULLUP);
  radioEnabled = !digitalRead(PIN_ENABLE_RADIO);

  if (!radioEnabled) {
    // Radio is disabled via jumper; don't touch the RF24 at all.
    Serial.println(F("Radio disabled (PIN_ENABLE_RADIO high)"));
    lcd.print(F("RADIO DISABLED"));
    delay(250);
    return;
  }

  // Power level jumper
  pinMode(PIN_PWR2_, INPUT_PULLUP);
  // Original design: 0=MIN, 1=LOW, 2=HIGH, 3=MAX
  rf24_pa_dbm_e power = (rf24_pa_dbm_e)(2 * digitalRead(PIN_PWR2_) + 1);

  radio.begin();

  if (!radio.isChipConnected()) {
    Serial.println(F("Radio fault"));
    lcd.clear();
    lcd.print(F("RADIO FAULT"));
    delay(500);
    radioEnabled = false;
    return;
  }

  radio.setDataRate(RF24_1MBPS);
  radio.setPALevel(power);
  radio.setChannel(CHANNEL_BASE);

  // Robust comms: auto-ack, retries, dynamic payloads and ACK payloads.
  radio.setAutoAck(true);
  radio.enableDynamicPayloads();
  radio.enableAckPayload();
  radio.setRetries(5, 15); // 5 retries, 15 * 250us between

  // Both nodes share the same address; master==display, slave==thermocouple.
  radio.openWritingPipe(DEVICE_ID);
  radio.openReadingPipe(1, DEVICE_ID);
  radio.startListening();

  isMasterNode = !xmitMode; // original: xmitMode true on TX node

  Serial.print(F("Radio OK, role: "));
  lcd.clear();
  if (isMasterNode) {
    Serial.println(F("MASTER (display)"));
    lcd.print(F("RADIO MASTER"));
  } else {
    Serial.println(F("SLAVE (sensor)"));
    lcd.print(F("RADIO SLAVE"));
  }
  lcd.setCursor(0, 1);
  lcd.print(F("CH "));
  lcd.print((int)CHANNEL_BASE);
  lcd.print(F(" PWR "));
  lcd.print((int)power);
  delay(750);
}

// Master side (LCD / receiver board):
// Send a ThermoReq and wait for a ThermoResp attached as an ACK payload.
// Returns true on success and fills outC with the remote temperature.
bool requestCelsius(float &outC) {
  if (!radioEnabled) {
    return false;
  }

  ThermoReq req;
  req.cmd      = 1;           // READ_TEMP
  req.reserved = 0;
  req.seq      = ++g_seqCounter;

  ThermoResp resp;
  const uint8_t MAX_TRIES = 3;

  for (uint8_t attempt = 0; attempt < MAX_TRIES; ++attempt) {
    radio.stopListening();
    bool ok = radio.write(&req, sizeof(req));
    radio.startListening();

    if (!ok) {
      // RF level failure even after auto-retries; try again.
      continue;
    }

    // If the slave attached an ACK payload, read it.
    if (radio.isAckPayloadAvailable()) {
      while (radio.isAckPayloadAvailable()) {
        radio.read(&resp, sizeof(resp));
      }

      if (resp.seq == req.seq && resp.status == 0) {
        outC = resp.tempC_x100 / 100.0f;
        return true;
      }
      // Wrong seq or error status; treat as failure and retry.
    } else {
      // Got an ACK with no payload; treat as failure and retry.
    }
  }

  return false; // no valid reply after bounded retries
}

// Slave side (thermocouple / transmitter board):
// Called frequently from loop() with the CURRENT local temperature.
// If a ThermoReq packet is present, consume it and attach a ThermoResp
// as an ACK payload. We do NOT actively transmit; the master pulls data
// by sending a request packet.
void processRadioAsSlave(float currentTemp) {
  if (!radioEnabled) {
    return;
  }

  if (!radio.available()) {
    return; // nothing to do
  }

  ThermoReq req;

  // Read the incoming request packet
  radio.read(&req, sizeof(req));

  if (req.cmd != 1) {
    // Unknown command; ignore.
    return;
  }

  ThermoResp resp;
  resp.status     = 0;
  resp.reserved   = 0;
  resp.seq        = req.seq;
  resp.tempC_x100 = (int32_t)(currentTemp * 100.0f);

  // Attach the reply as an ACK payload for this pipe.
  // The RF24 hardware will send it back to the master as part of the ACK.
  radio.writeAckPayload(1, &resp, sizeof(resp));
}
