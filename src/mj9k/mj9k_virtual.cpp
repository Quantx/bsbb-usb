#include <cstdint>
#include <cstring>
#include <tusb.h>
#include <Arduino.h>

#include "mj9k.h"
#include "xidd.h"

static mj9k_virtual * head, ** tail = &head;

mj9k_virtual::mj9k_virtual(uint8_t p_interface) {
    interface = p_interface;

    next = nullptr;
    *tail = this;
    tail = &next;
}

mj9k_virtual::~mj9k_virtual() {
    for (mj9k_virtual ** curr = &head; curr; curr = &(*curr)->next) {
        if (*curr == this) {
            if (tail == &next) {
                tail = curr;
            }
            *curr = this->next;
        }
    }
}

void mj9k_virtual::set_button(mj9k::Button p_button, bool pressed) {
    unsigned int button_idx = p_button;
    if (button_idx >= MJ9K_BUTTON_COUNT) return;

    unsigned int button_offset = button_idx / 16;
    uint16_t button_mask = 1 << (button_idx % 16);

    if (pressed) {
        status_in.buttons[button_offset] |= button_mask;
    } else {
        status_in.buttons[button_offset] &= ~button_mask;
    }
}

void mj9k_virtual::set_aiming(float x, float y) {
    x = constrain(x, 0.0f, 1.0f);
    y = constrain(y, 0.0f, 1.0f);

    status_in.aimingX = x * (float)0xFFFF;
    status_in.aimingY = y * (float)0xFFFF;
}

void mj9k_virtual::set_turning(float value) {
    value = constrain(value, -1.0f, 1.0f);

    status_in.turningLever = value * (float)0x7FFF;
}

void mj9k_virtual::set_sight(float x, float y) {
    x = constrain(x, -1.0f, 1.0f);
    y = constrain(y, -1.0f, 1.0f);

    status_in.sightChangeX = x * (float)0x7FFF;
    status_in.sightChangeY = y * (float)0x7FFF;
}

void mj9k_virtual::set_pedal(enum Pedal p_pedal, float value) {
    value = constrain(value, 0.0f, 1.0f);
    value *= (float)0xFFFF;
    switch (p_pedal) {
        case PEDAL_ACCEL: status_in.accelPedal = value; break;
        case PEDAL_BRAKE: status_in.brakePedal = value; break;
        case PEDAL_SLIDE: status_in.slidePedal = value; break;
    }
}

void mj9k_virtual::set_tuner(uint8_t value) {
    status_in.tuner = value & 0xF;
}

void mj9k_virtual::set_shifter(int8_t value) {
    status_in.shifter = constrain(value, -1, 5);
}

void mj9k_virtual::copy_input_from(mj9k &controller) {
    controller.read_status_in(&status_in);
}

int8_t mj9k_virtual::get_light(mj9k::Light p_light) const {
    unsigned int light_idx = p_light;
    if (light_idx >= MJ9K_LIGHT_COUNT) return -1;

    const uint8_t * light = status_out.lights + light_idx / 2;

    return (light_idx % 2 ? *light : *light >> 4) & 0xF;
}

bool mj9k_virtual::is_connected(void) {
    return xidd_is_interface_open(interface);
}

bool mj9k_virtual::transmit_input(void) {
    if (interface == 0xFF) {
        return false;
    }

    return xidd_send_report(interface, &status_in, sizeof(mj9k_in_report));
}

void mj9k_virtual::handle_out_report(uint8_t itf_num, void * ep_out, uint8_t xferred_bytes) {
    if (xferred_bytes != sizeof(mj9k_out_report)) {
        return;
    }

    for (mj9k_virtual * controller = head; controller; controller = controller->next) {
        if (itf_num == controller->interface) {
            memcpy(&controller->status_out, ep_out, sizeof(mj9k_out_report));
        }
    }
}
