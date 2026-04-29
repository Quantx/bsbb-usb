#ifndef BSBB_MJ9K_H
#define BSBB_MJ9K_H

#include <cstdint>
#include <list>

// Corresponds to: xid_descriptor_t.bType
#define XID_TYPE_MJ9K 0x80
// Corresponds to: xid_descriptor_t.bSubType
#define XID_SUBTYPE_MJ9K 0x01

#define MJ9K_LIGHT_COUNT 40
#define MJ9K_BUTTON_COUNT 48

typedef struct __attribute__((packed)) {
    uint8_t zero;
    uint8_t bLength;
    uint16_t buttons[MJ9K_BUTTON_COUNT / 16];
    uint16_t aimingX;	// 0 to 0xFFFF left to right
    uint16_t aimingY;	// 0 to 0xFFFF top to bottom
    int16_t turningLever;
    int16_t sightChangeX;
    int16_t sightChangeY;
    uint16_t slidePedal;	// Sidestep, 0x0000 to 0xFF00
    uint16_t brakePedal;	// Brake, 0x0000 to 0xFF00
    uint16_t accelPedal;	// Acceleration, 0x0000 to oxFF00
    uint8_t tuner;          // 0-15 is from 9oclock, around clockwise
    int8_t shifter;         // -2 = R, -1 = N, 0 = Error, 1 = 1st, 2 = 2nd, 3 = 3rnd, 4 = 4th, 5 = 5th
} mj9k_in_report;

typedef struct __attribute__((packed)) {
    uint8_t zero;
    uint8_t bLength;
    uint8_t lights[MJ9K_LIGHT_COUNT / 2];
} mj9k_out_report;

class mj9k {
protected:
    mj9k_in_report status_in;
    mj9k_out_report status_out;

    mj9k(void) {
        status_in = (mj9k_in_report){.bLength = sizeof(mj9k_in_report)};
        status_out = (mj9k_out_report){.bLength = sizeof(mj9k_out_report)};
    }
public:
    enum Pedal : uint8_t {
        PEDAL_ACCEL,
        PEDAL_BRAKE,
        PEDAL_SLIDE
    };

    enum Button : uint8_t {
        BUTTON_FIRE_MAIN,
        BUTTON_FIRE_SUB,
        BUTTON_LOCK_ON,
        BUTTON_EJECT,
        BUTTON_HATCH,
        BUTTON_IGNITION,
        BUTTON_START,
        BUTTON_MULTI_TOGGLE,
        BUTTON_MULTI_ZOOM,
        BUTTON_MULTI_MODE,
        BUTTON_SUB_MODE,
        BUTTON_ZOOM_IN,
        BUTTON_ZOOM_OUT,
        BUTTON_FSS,
        BUTTON_MANIPULATOR,
        BUTTON_LINE_COLOR_CHANGE,
        BUTTON_WASHING,
        BUTTON_EXTINGUISHER,
        BUTTON_CHAFF,
        BUTTON_TANK_DETACH,
        BUTTON_OVERRIDE,
        BUTTON_NIGHT_SCOPE,
        BUTTON_F1,
        BUTTON_F2,
        BUTTON_F3,
        BUTTON_CYCLE_MAIN,
        BUTTON_CYCLE_SUB,
        BUTTON_MAG_CHANGE,
        BUTTON_COMM_1,
        BUTTON_COMM_2,
        BUTTON_COMM_3,
        BUTTON_COMM_4,
        BUTTON_COMM_5,
        BUTTON_SIGHT_CHANGE,
        // Toggle Switches
        SWITCH_FILTER,
        SWITCH_OXYGEN,
        SWITCH_FUEL,
        SWITCH_BUFFER,
        SWITCH_LOCATION,

        BUTTON_MAX,
    };

    enum Light : uint8_t {
        LIGHT_EJECT,
        LIGHT_HATCH,
        LIGHT_IGNITION,
        LIGHT_START,
        LIGHT_MULTI_TOGGLE,
        LIGHT_MULTI_ZOOM,
        LIGHT_MULTI_MODE,
        LIGHT_SUB_MODE,
        LIGHT_ZOOM_IN,
        LIGHT_ZOOM_OUT,
        LIGHT_FSS,
        LIGHT_MANIPULATOR,
        LIGHT_LINE_COLOR_CHANGE,
        LIGHT_WASHING,
        LIGHT_EXTINGUISHER,
        LIGHT_CHAFF,
        LIGHT_TANK_DETACH,
        LIGHT_OVERRIDE,
        LIGHT_NIGHT_SCOPE,
        LIGHT_F1,
        LIGHT_F2,
        LIGHT_F3,
        LIGHT_CYCLE_MAIN,
        LIGHT_CYCLE_SUB,
        LIGHT_MAG_CHANGE,
        LIGHT_COMM_1,
        LIGHT_COMM_2,
        LIGHT_COMM_3,
        LIGHT_COMM_4,
        LIGHT_COMM_5,
        LIGHT_GEAR_R = LIGHT_COMM_5 + 2,
        LIGHT_GEAR_N,
        LIGHT_GEAR_1,
        LIGHT_GEAR_2,
        LIGHT_GEAR_3,
        LIGHT_GEAR_4,
        LIGHT_GEAR_5,

        LIGHT_MAX,
    };

    static enum Button light_get_button(enum Light light);

    void read_status_in(mj9k_in_report * status) const;
    void read_status_out(mj9k_out_report * status) const;
};

// This represents a genuine physical MJ9K controller connected to a the USB Host port
class mj9k_physical : public mj9k {
private:
    // The physical USB Hub port that corresponds to this controller
    uint8_t port;
    mj9k_physical * next;
public:
    mj9k_physical(uint8_t p_port);
    ~mj9k_physical();

    static void handle_in_report(uint8_t daddr, void * ep_in, uint8_t xferred_bytes);

    bool is_connected(void);

    // Controller input
    bool get_button(enum Button button) const;
    void get_aiming(float * x, float * y) const;
    float get_turning(void) const;
    void get_sight(float * x, float * y) const;
    float get_pedal(enum Pedal pedal) const;
    uint8_t get_tuner(void) const;
    int8_t get_shifter(void) const;

    // Controller lights
    void set_light(enum Light light, uint8_t brightness);

    void copy_lights_from(mj9k &controller);
    bool transmit_lights(void);
};

// This represents a fake MJ9K controller which is present to the Xbox as a USB Device
class mj9k_virtual : public mj9k {
private:
    // The USB interface bound to this controller, 0xFF = not bound to any interface
    uint8_t interface;
    mj9k_virtual * next;
public:
    mj9k_virtual(uint8_t interface);
    ~mj9k_virtual();

    static void handle_out_report(uint8_t itf_num, void * ep_out, uint8_t xferred_bytes);

    bool is_connected(void);

    // Controller input
    void set_button(enum Button button, bool pressed);
    void set_aiming(float x, float y); // 0 <= x/y <= 1
    void set_turning(float value); // -1 <= value <= 1
    void set_sight(float x, float y); // -1 <= x/y <= 1
    void set_pedal(enum Pedal pedal, float value); // 0 <= value <= 1
    void set_tuner(uint8_t value); // 0 <= value <= 15
    void set_shifter(int8_t value); // -1 <= value <= 5

    void copy_input_from(mj9k &controller);
    bool transmit_input(void);

    // Controller lights
    int8_t get_light(enum Light light) const; // -1 if an invalid light enum is provided, otherwise 0 - 15 brightness value
};

#endif
