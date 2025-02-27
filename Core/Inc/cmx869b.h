//
// Created by kikuchi on 25/02/14.
//

#ifndef CMX869B_CMX869B_H
#define CMX869B_CMX869B_H

#define _MODE_LOOP

#include "stm32f3xx_hal.h"
void CMX869B_Init();
int CMX869B_Receive(uint8_t *rx_data);
int CMX869B_Transmit(uint8_t tx_data);
int CMX869B_is_gse();
#endif //CMX869B_CMX869B_H
