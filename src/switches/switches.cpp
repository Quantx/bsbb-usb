#include <Arduino.h>
#include <stdint.h>

#include "switches.h"

uint16_t switches; // extern
uint16_t switches_last; // extern

unsigned long switches_timer;
uint16_t switches_poll;

// Outputs
const int switches_column_start = 34;
const int switches_column_count = 4;

// Inputs
const int switches_row_start = 38;
const int switches_row_count = 4;

uint8_t switches_column_current;

void switches_setup(void) {
    for (int r = 0; r < switches_row_count; r++) {
        pinMode(switches_row_start + r, INPUT_PULLUP);
    }
    
    for (int c = 0; c < switches_column_count; c++) {
        int switches_column_addr = switches_column_start + c;
        pinMode(switches_column_addr, OUTPUT_OPENDRAIN);
        digitalWrite(switches_column_addr, c ? LOW : HIGH);
    }
}

void switches_loop(void) {
    unsigned long now = millis();
    if (switches_timer > now) return;
    switches_timer = now + 50; // Poll at 20Hz
    
    // Read each row in this column
    for (int r = 0; r < switches_row_count; r++) {
        int switches_row_addr = switches_row_start + r;
        switches_poll <<= 1;
        switches_poll |= (~digitalRead(switches_row_addr) & 1);
    }
    
    // Set next active column
    for (int c = 0; c < switches_column_count; c++) {
        int switches_column_addr = switches_column_start + c;
        digitalWrite(switches_column_addr, c == switches_column_current ? LOW : HIGH);
    }
    
    switches_column_current++;
    if (switches_column_current >= switches_column_count) {
        // Finished polling all switches, apply them to the outout and reset
        switches_last = switches;
        switches = switches_poll;
        switches_poll = 0;
        switches_column_current = 0;
    }
}
