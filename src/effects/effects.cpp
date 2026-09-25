#include "effects.h"

void effects_setup(void) {
    Wire.begin();
}

void effects_loop(void) {
    const int i2c_addr = 0x20; // Address format = MSB[0, 0, 1, 0, 0, A2, A1, A0]LSB (RW = 0)

    if (!relay_loop) relay_loop = ~1;

    Wire.beginTransmission(0x20);
    Wire.write(0x00);  // register address
    Wire.write(0x0F);  // IODIRA
    Wire.write(0x0F);  // IODIRB
    Wire.endTransmission();
    
    Wire.beginTransmission(0x20);
    Wire.write(0x12); // register address
    Wire.write(relay_loop & 0x00F0);  // GPIOA
    Wire.write((relay_loop & 0x000F) << 4);  // GPIOB
    Wire.endTransmission();
    
    Wire.beginTransmission(0x21);
    Wire.write(0x00);  // register address
    Wire.write(0x0F);  // IODIRA
    Wire.write(0x0F);  // IODIRB
    Wire.endTransmission();
    
    Wire.beginTransmission(0x21);
    Wire.write(0x12); // register address
    Wire.write((relay_loop & 0x0F00) >> 4);  // GPIOA
    Wire.write((relay_loop & 0xF000) >> 8);  // GPIOB
    Wire.endTransmission();
    
    relay_loop <<= 1;
    delay(500);
    
    /*
    Wire.requestFrom(i2c_addr, 0x1B);
    bool got_any = false;
    for (uint32_t addr = 0; Wire.available(); addr++) {
      uint8_t val = Wire.read();
      got_any = true;
    }
    digitalWrite(led_cockpit, got_any);
    */
}
