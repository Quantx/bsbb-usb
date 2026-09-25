#ifndef BSBB_SWITCHES_H
#define BSBB_SWITCHES_H

#include <stdint.h>

enum BSBBSwitch : uint16_t {
    ControllerCoach = 0x0001,
    ControllerPilot = 0x0002,
    Pause           = 0x0004,
    Resume          = 0x0008,
    TargetLimiter   = 0x0010,
    HatDisable      = 0x0020,
    Inversion       = 0x0040,
    AutoAccelerate  = 0x0080,
    EffectFull      = 0x0100,
    EffectDisable   = 0x0200,
    KeySwitch       = 0x0400,
    Unused0         = 0x0800,
    Unused1         = 0x1000,
    Unused2         = 0x2000,
    Unused3         = 0x4000,
    Unused4         = 0x8000,
};

extern uint16_t switches;
extern uint16_t switches_last;

void switches_setup(void);
void switches_loop(void);

#endif
