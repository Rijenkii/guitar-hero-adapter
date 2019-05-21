// Try to identify broken state packets
#define VERIFY 0

#include <Wire.h>
#include "Joystick.h"

Joystick_ Joystick(
  JOYSTICK_DEFAULT_REPORT_ID, // hidReportId
  JOYSTICK_TYPE_GAMEPAD,      // joystickType
  9,                          // buttonCount
  0,                          // hatSwitchCount
  true,                       // includeXAxis
  true,                       // includeYAxis
  true,                       // includeZAxis
  false,                      // includeRxAxis
  false,                      // includeRyAxis
  false,                      // includeRzAxis
  false,                      // includeRudder
  false,                      // includeThrottle
  false,                      // includeAccelerator
  false,                      // includeBrake
  false                       // includeSteering
);

#define ADDRESS 0x52

void send_byte(byte data, byte location) {
  Wire.beginTransmission(ADDRESS);
  Wire.write(location);
  Wire.write(data);
  Wire.endTransmission();
  delay(1);
}

void read_state(byte data[6]) {
  send_byte(0, 0);
  
  // Read state from the guitar
  byte count = 0;
  Wire.requestFrom(ADDRESS, 6);
  while (Wire.available())
    data[count++] = Wire.read();
}

#if VERIFY
byte GH3;
byte ZERO;
#endif

void setup() {
  // Initizlize I2C
  //
  // Wiitars should work at speeds up to 400000 bits/sec, but mine does not
  // But we only read 6 bytes at a time, so I don't think that it really maters
  Wire.begin();
  Wire.setClock(350000);

  // Initialize joystick
  Joystick.begin(false);
  Joystick.setXAxisRange(0, 63);
  Joystick.setYAxisRange(0, 63);
  Joystick.setZAxisRange(0, 31);

  // Initialize unencrypted connection
  send_byte(0x55, 0xF0);
  send_byte(0x00, 0xFB);

#if VERIFY
  // Identify what bits should be at GH3 and "0" positions
  // http://www.wiibrew.org/wiki/Wiimote/Extension_Controllers/Guitar_Hero_(Wii)_Guitars
  byte temp1 = 0;
  byte temp2 = 0;
  byte data[6];
  for (byte _ = 0; _ < 20; _++) {
    read_state(data);
    
    temp1 += ((data[0] >> 7) & 1) + ((data[0] >> 6) & 1);
    temp1 += ((data[1] >> 7) & 1) + ((data[1] >> 6) & 1);

    temp2 += ((data[2] >> 7) & 1) + ((data[2] >> 6) & 1) + ((data[2] >> 5) & 1);
    temp2 += ((data[3] >> 7) & 1) + ((data[3] >> 6) & 1) + ((data[3] >> 5) & 1);
  }
  GH3 = temp1 > 40 ? 0b11000000 : 0b0000000;
  ZERO = temp2 > 60 ? 0b11100000 : 0b0000000;
#endif
}

byte data[6];

void loop() {
  // Request state from the guitar
  read_state(data);

#if VERIFY
  if (
    (data[0] & 0b11000000) != GH3 ||
    (data[1] & 0b11000000) != GH3 ||
    (data[2] & 0b11100000) != ZERO ||
    (data[3] & 0b11100000) != ZERO ||
    (data[4] & 0b10101011) != 0b10101011 ||
    (data[5] & 0b00000110) != 0b00000110
  ) {
    return;
  }
#endif

  Joystick.setButton(0, !(data[5] & 0b00010000)); // Fret green
  Joystick.setButton(1, !(data[5] & 0b01000000)); // Fret red
  Joystick.setButton(2, !(data[5] & 0b00001000)); // Fret yellow
  Joystick.setButton(3, !(data[5] & 0b00100000)); // Fret blue
  Joystick.setButton(4, !(data[5] & 0b10000000)); // Fret orange

  Joystick.setButton(5, !(data[4] & 0b01000000)); // Strum down
  Joystick.setButton(6, !(data[5] & 0b00000001)); // Strum up

  Joystick.setButton(7, !(data[4] & 0b00010000)); // Button minus
  Joystick.setButton(8, !(data[4] & 0b00000100)); // Button plus

  Joystick.setXAxis(data[0] & 0b00111111); // Stick X
  Joystick.setYAxis(data[1] & 0b00111111); // Stick Y
  Joystick.setZAxis(data[3] & 0b00011111); // Whammy

  Joystick.sendState();
}
