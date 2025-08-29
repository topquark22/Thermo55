#import "thermo55.h"

void setOutput(bool value) {
  digitalWrite(PIN_OUT, value);
  digitalWrite(PIN_OUT_, !value);
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
