//
// Created by kikuchi on 2026/09/27.
//
#include "string.h"
#include "main.h"
#include "CMX869b.h"
#include "HK.h"
#include "xprintf.h"
//
extern uint8_t TX_buffer[];
extern uint8_t RX_buffer[];
extern int TX_ptr;
extern int RX_ptr;
//
//********************************************
// HK生成とか
//********************************************
void HKLoop() {
    int i=0;
    for (;;) {
        xprintf("HKLoop %08d\r\n", i++);
        TX_ptr = 1;
        cbus_write(TxData_ADDR, 0x01);
        HAL_Delay(1000);
    }
}
