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
        CMX869B_poll();
        HAL_Delay(5000);
        nn++;
    }
}
