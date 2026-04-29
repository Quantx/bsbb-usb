#include <cstdint>
#include <cstring>

#include "mj9k.h"
#include "xidh.h"

static mj9k_physical * head, ** tail = &head;

mj9k_physical::mj9k_physical(uint8_t p_port) {
    port = p_port;

    next = nullptr;
    *tail = this;
    tail = &next;
}

mj9k_physical::~mj9k_physical() {
    for (mj9k_physical ** curr = &head; curr; curr = &(*curr)->next) {
        if (*curr == this) {
            if (tail == &next) {
                tail = curr;
            }
            *curr = this->next;
        }
    }
}

void mj9k_physical::set_light(mj9k::Light p_light, uint8_t p_brightness) {
    unsigned int light_idx = p_light;
    if (light_idx >= MJ9K_LIGHT_COUNT) return;

    uint8_t * light = status_out.lights + light_idx / 2;
    uint8_t value = p_brightness & 0xF;

    if (light_idx % 2) {
        *light = (value << 4) | (*light & 0xF);
    } else {
        *light = value | (*light & 0xF0);
    }
}

bool mj9k_physical::get_button(mj9k::Button p_button) const {
    unsigned int button_idx = p_button;
    if (button_idx >= MJ9K_BUTTON_COUNT) return false;

    unsigned int button_offset = button_idx / 16;
    uint16_t button_mask = 1 << (button_idx % 16);

    return status_in.buttons[button_offset] & button_mask;
}

void mj9k_physical::get_aiming(float * x, float * y) const {
    if (x) *x = (float)status_in.aimingX / (float)0xFFFF;
    if (y) *y = (float)status_in.aimingY / (float)0xFFFF;
}

float mj9k_physical::get_turning() const {
    return (float)status_in.turningLever / (float)0x7FFF;
}

void mj9k_physical::get_sight(float * x, float * y) const {
    if (x) *x = (float)status_in.sightChangeX / (float)0x7FFF;
    if (y) *y = (float)status_in.sightChangeY / (float)0x7FFF;
}

float mj9k_physical::get_pedal(mj9k::Pedal p_pedal) const {
    float value = 0.0;
    switch (p_pedal) {
        case PEDAL_ACCEL: value = status_in.accelPedal; break;
        case PEDAL_BRAKE: value = status_in.brakePedal; break;
        case PEDAL_SLIDE: value = status_in.slidePedal;	break;
    }

    return value / (float)0xFFFF;
}

uint8_t mj9k_physical::get_tuner() const {
    return status_in.tuner;
}

int8_t mj9k_physical::get_shifter() const {
    return status_in.shifter;
}

void mj9k_physical::copy_lights_from(mj9k &controller) {
    controller.read_status_out(&status_out);
}

bool mj9k_physical::is_connected(void) {
    return xidh_dev_addr_from_port(port);
}

bool mj9k_physical::transmit_lights(void) {
    uint8_t daddr = xidh_dev_addr_from_port(port);
    if (!daddr) {
        return false;
    }

    return xidh_send_report(daddr, &status_out, sizeof(mj9k_out_report));
}

void mj9k_physical::handle_in_report(uint8_t daddr, void * ep_in, uint8_t xferred_bytes) {
    if (xferred_bytes != sizeof(mj9k_in_report)) {
        return;
    }

    for (mj9k_physical * controller = head; controller; controller = controller->next) {
        if (daddr == xidh_dev_addr_from_port(controller->port)) {
            memcpy(&controller->status_in, ep_in, sizeof(mj9k_in_report));
        }
    }
}
