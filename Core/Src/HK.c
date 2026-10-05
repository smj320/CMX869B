//
// Created by kikuchi on 2026/09/27.
//
#include "string.h"
#include "main.h"
#include "CMX869b.h"
#include "HK.h"
#include "xprintf.h"
//
extern TIM_HandleTypeDef htim2;

extern uint8_t TX_buffer[];
extern uint8_t RX_buffer[];
extern int TX_ptr;
extern int RX_ptr;

//
//********************************************
// HK生成とか
//********************************************
void HKLoop() {
    static int count = 0;

    for (;;) {
        CBUS_DATA_WRITE('0'+count++%10);
        HAL_Delay(20);
    }
}
