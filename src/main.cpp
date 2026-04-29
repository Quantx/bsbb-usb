#include <tusb.h>
#include <Arduino.h>

#include "usb/usb.h"
#include "mj9k/mj9k.h"

mj9k_virtual xbox_ctrl(0);
mj9k_physical cockpit_ctrl(1);
mj9k_physical coach_ctrl(2);

void setup() {
    SERDBG.begin(115200);
    printf("\r\n*** BSBB USB multiplexer start ***\r\n");

    pinMode(LED_BUILTIN, OUTPUT);

    usb_setup();
}

void loop(void) {
    usb_loop();

    cockpit_ctrl.copy_lights_from(xbox_ctrl);
    xbox_ctrl.copy_input_from(cockpit_ctrl);

    mj9k_in_report status;
    xbox_ctrl.read_status_in(&status);

    printf("TURNING LEVER %d\r\n", status.turningLever);

    if (cockpit_ctrl.is_connected()) {
        if (!cockpit_ctrl.transmit_lights()) {
            printf("Failed to transmit lights\r\n");
        }
    }

    if (xbox_ctrl.is_connected()) {
        if (xbox_ctrl.transmit_input()) {
            //printf("Successfully transmited input\r\n");
        } else {
            //printf("Failed to transmit input\r\n");
        }
    }
}
