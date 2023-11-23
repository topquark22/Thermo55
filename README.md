# Temperature Alerting System

## Hardware requirements

- Type K thermocouple wire
- AdaFruit MAX31855 thermocouple amplifier breakout board
- LCD 1602 display with I2C capability

## Attaching a Type-K Thermocouple

Identify the wires:
- Type-K thermocouple wires are typically color-coded.
- The negative wire is usually red, and the positive wire can be yellow or green, depending on the standard.
- In a case where there's only one wire visible, it's typically encased with the other in a single insulation.

Attaching the hot junction:
- The hot junction is the part of the thermocouple that gets exposed to the temperature you want to measure.
- Use thermal compound to improve thermal contact if necessary.
- Be cautious about using thermal compound that can harden or is electrically conductive, especially if you plan to detach and reattach the thermocouple.

Securing the thermocouple:
- Use Kapton tape for attaching the thermocouple to a surface like a transistor. It's heat resistant and leaves minimal residue.
- The adhesive used in Kapton tape is typically a silicone adhesive which can withstand high temperatures.

## Testing the MAX31855 Breakout Board

On the Arduino Nano, MISO is pin 12 and SCK is pin 13.

Any digital pin can be used for CS. In this project we use D8.

Connecting the Adafruit breakout board:
- Connect +5V to Vin
- Connect GND to ground.
- Connect the SPI pins (CS, SCK, MISO) to the corresponding pins on the Arduino.

Testing for open circuits:
- Use the `readError()` method from the Adafruit MAX31855 library to check for any faults with the thermocouple connections.

## Some components

Not yet connected

![Breakout board 1](thermo1.jpg)

Connected to Arduino, breakout board working without thermocouple attached. Correctly displays open circuit error condition

![incomplete 1](open-circuit-test-working.jpg)

Soldered up and attached to the thermocouple. It is working as expected.

![prototype](thermo3.jpg)

## I2C interface

Successfully implemented using the I2C interface. Note the board is wired up for the parallel interface, but it just so happens that the same pins required for I2C are available on the parallel interface pin header, so I just repurposed those. SDA is connected to A4 (purple on board, yellow Dupont wire). SCL is connected to A5 (green on the board, orange Dupont wire).

![I2C working](working-I2C.jpg)

![I2C working2](working2-I2C.jpg)
