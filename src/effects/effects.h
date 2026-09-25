#ifndef BSBB_EFFECTS_H
#define BSBB_EFFECTS_H

#include <stdint.h>

enum BSBBEffect : uint16_t {
    Effect0 = 0x0001,
    Effect1 = 0x0002,
    Effect2 = 0x0004,
    Effect3 = 0x0008,
    Effect4 = 0x0010,
    Effect5 = 0x0020,
    Effect6 = 0x0040,
    Effect7 = 0x0080,
    Effect8 = 0x0100,
    Effect9 = 0x0200,
    Effect10 = 0x0400,
    Effect11 = 0x0800,
    Effect12 = 0x1000,
    Effect13 = 0x2000,
    Effect14 = 0x4000,
    Effect16 = 0x8000,
};

void effects_setup(void);
void effects_loop(void);
#endif
