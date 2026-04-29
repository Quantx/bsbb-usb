#include <cstdint>
#include <cstring>

#include "mj9k.h"

void mj9k::read_status_in(mj9k_in_report * status) const {
    if (status) memcpy(status, &status_in, sizeof(mj9k_in_report));
}

void mj9k::read_status_out(mj9k_out_report * status) const {
    if (status) memcpy(status, &status_out, sizeof(mj9k_out_report));
}

static const enum mj9k::Button light_to_buttons[30] = {
    mj9k::BUTTON_EJECT,
    mj9k::BUTTON_HATCH,
    mj9k::BUTTON_IGNITION,
    mj9k::BUTTON_START,
    mj9k::BUTTON_MULTI_TOGGLE,
    mj9k::BUTTON_MULTI_ZOOM,
    mj9k::BUTTON_MULTI_MODE,
    mj9k::BUTTON_SUB_MODE,
    mj9k::BUTTON_ZOOM_IN,
    mj9k::BUTTON_ZOOM_OUT,
    mj9k::BUTTON_FSS,
    mj9k::BUTTON_MANIPULATOR,
    mj9k::BUTTON_LINE_COLOR_CHANGE,
    mj9k::BUTTON_WASHING,
    mj9k::BUTTON_EXTINGUISHER,
    mj9k::BUTTON_CHAFF,
    mj9k::BUTTON_TANK_DETACH,
    mj9k::BUTTON_OVERRIDE,
    mj9k::BUTTON_NIGHT_SCOPE,
    mj9k::BUTTON_F1,
    mj9k::BUTTON_F2,
    mj9k::BUTTON_F3,
    mj9k::BUTTON_CYCLE_MAIN,
    mj9k::BUTTON_CYCLE_SUB,
    mj9k::BUTTON_MAG_CHANGE,
    mj9k::BUTTON_COMM_1,
    mj9k::BUTTON_COMM_2,
    mj9k::BUTTON_COMM_3,
    mj9k::BUTTON_COMM_4,
    mj9k::BUTTON_COMM_5,
};

enum mj9k::Button mj9k::light_get_button(enum Light light) {
    if (light >= sizeof(light_to_buttons) / sizeof(*light_to_buttons)) {
        return mj9k::BUTTON_MAX;
    }

    return light_to_buttons[light];
}
