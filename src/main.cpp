#include <tusb.h>
#include <Arduino.h>
#include <Wire.h>

#include "usb/usb.h"
#include "mj9k/mj9k.h"
#include "switches/switches.h"

mj9k_virtual xbox_ctrl(0);
mj9k_physical coach_ctrl(1);
mj9k_physical cockpit_ctrl(2);

// Map a value from the (0.0 - 1.0) range to a custom (min - max) range
float map(float value, float min, float max) {
    return min + (value) * (max - min);
}

const int led_cockpit = 23;
const int led_switches = 33;

uint16_t relay_loop;

void setup() {
    SERDBG.begin(115200);
    printf("\r\n*** BSBB USB multiplexer start ***\r\n");

    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(led_cockpit, OUTPUT);
    pinMode(led_switches, OUTPUT);
    
    switches_setup();
    effects_setup();
    usb_setup();
}

void loop(void) {
    /*
    usb_loop();

    cockpit_ctrl.copy_lights_from(xbox_ctrl);
    coach_ctrl.copy_lights_from(xbox_ctrl);
    
    bool disable = coach_ctrl.get_button(mj9k::BUTTON_COMM_5);
    usb_set_connected(!disable);
    
    float from_x, from_y;
    
    bool coach_owner = coach_ctrl.get_button(mj9k::SWITCH_LOCATION);
    if (coach_owner) {
        xbox_ctrl.copy_input_from(coach_ctrl);
        coach_ctrl.get_aiming(&from_x, &from_y);
    } else {
        xbox_ctrl.copy_input_from(cockpit_ctrl);
        cockpit_ctrl.get_aiming(&from_x, &from_y);
    }
    
    float to_x = map(from_x, 0.445, 0.64);
    float to_y = map(from_y, 0.45, 0.8);
    
    xbox_ctrl.set_aiming(to_x, to_y);
    
    bool coach_any_pressed = false;
    for (int i = 0; i < mj9k::LIGHT_MAX; i += 1) {
        mj9k::Light light = static_cast<mj9k::Light>(i);
        mj9k::Button button = mj9k::light_get_button(light);
        if (button != mj9k::BUTTON_MAX && coach_ctrl.get_button(button)) {
            coach_any_pressed = true;
            break;
        }
    }
    
    if (coach_any_pressed && !coach_owner) {
        for (int i = 0; i < mj9k::LIGHT_MAX; i += 1) {
            mj9k::Light light = static_cast<mj9k::Light>(i);
            mj9k::Button button = mj9k::light_get_button(light);
            if (button != mj9k::BUTTON_MAX) {
                if (coach_ctrl.get_button(button)) {
                    cockpit_ctrl.set_light(light, 0xF);
                } else {
                    cockpit_ctrl.set_light(light, 0x0);
                }
            }
        }
    }
    
    if (cockpit_ctrl.is_connected()) {
        if (!cockpit_ctrl.transmit_lights()) {
            printf("Failed to transmit cockpit lights\r\n");
        }
    }
    
    if (coach_ctrl.is_connected()) {
        if (!coach_ctrl.transmit_lights()) {
            printf("Failed to transmit coach lights\r\n");
        }
    }

    if (xbox_ctrl.is_connected()) {
        if (xbox_ctrl.transmit_input()) {
            //printf("Successfully transmited input\r\n");
        } else {
            //printf("Failed to transmit input\r\n");
        }
    }
    */
    
    switches_loop();
    digitalWrite(led_switches, switches != 0);
    
    printf("Switches:");
    for (uint8_t i = 0; i < 16; i++) {
        if (switches & (1 << i)) {
            printf(" X");
        } else {
            printf(" O");
        }
    }
    printf("\r\n");
    
    effects_loop();
}
