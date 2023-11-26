#ifndef T
#define T

// alarm threshold supported range in degrees C
// (Note: Type K thermocouple actually supports -200 to +1300)
const float TEMP_LOW = -40;
const float TEMP_HIGH = 110;

// LCD I2C address and size
const int LCD_I2C_ADDR = 0x27;
const int LCD_WIDTH = 16;
const int LCD_HEIGHT = 2;

// interval between readings
const int INTERVAL = 1000;

// return the max/min from this many seconds earlier
#define LAG_TIME 15

// time backlight stays on
#define BACKLIGHT_TIME 7

#endif