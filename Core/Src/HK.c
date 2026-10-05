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
    static uint8_t flame[N_TX_BUFFER] = {0};

    for (;;) {
        for (int i=0;i<N_TX_BUFFER;i++) {
            flame[i]='0'+i%10;
        }
        CMX869B_write_buffered(flame,N_TX_BUFFER);
        HAL_Delay(1000);
    }
}
