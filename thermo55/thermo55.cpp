#import "thermo55.h"

void setOutput(bool value) {
  if (value) {
    // enable the auxiliary output the first time the threshold is triggered high
    enableAuxOutput(true);
  }
  digitalWrite(PIN_OUT, value);
}

void enableAuxOutput(bool value) {
  digitalWrite(PIN_AUX_ENABLE, value);
}

// never returns
void blinkLED(int millis) {
  while (true) {
    setOutput(HIGH);
    delay(millis);
    setOutput(LOW);
    delay(millis);
  }
}
