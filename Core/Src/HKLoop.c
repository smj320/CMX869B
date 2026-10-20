//
// Created by kikuchi on 2026/09/27.
//
#include "string.h"
#include "main.h"
#include "HKLoop.h"
#include "CMX869b.h"
#include "xprintf.h"
//
//
//********************************************
// データ送受信
//********************************************
void HKLoop(int is_gse) {
    uint8_t TxBuffer[N_TXBuf];
    uint8_t RxBuffer[N_RXBuf];
    int nn=0;

    CMX869B_Init(is_gse);

    //Drillの場合はフレーム転送モード
    for (;;) {
        //
        CMX869B_putc(nn);
        HAL_Delay(200);
        uint8_t c = CMX869B_getc();
        if (c != 0) {
            int id = is_gse*100+nn%10;
            xprintf("Received: %03d\r\n", id);
        }
        HAL_Delay(5000);
        nn++;
    }
}
