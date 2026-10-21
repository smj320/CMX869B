//
// Created by kikuchi on 2026/10/22.
//

#ifndef CMX869B_CBUS_HPP
#define CMX869B_CBUS_HPP

#include "cstdint"

extern "C" void cbus_entry(void *argument);

class cbus {
public:
    static uint8_t getc();

    static void putc(uint8_t c);
};


#endif //CMX869B_CBUS_HPP
