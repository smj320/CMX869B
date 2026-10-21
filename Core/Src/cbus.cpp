//
// Created by kikuchi on 2026/10/22.
//

#include "../Inc/cbus.hpp"
#include "xprintf.h"

void cbus::putc(uint8_t c) {
    xprintf("%c", c);
}

uint8_t cbus::getc() {
    xprintf("getc");
    return 0;
}

void cbus_entry(void *argument) {
    xprintf("cbus_entry");
}