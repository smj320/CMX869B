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
        memset(TxBuffer, nn%10, sizeof(TxBuffer));
        CMX869b_write(TxBuffer, sizeof(TxBuffer));
        HAL_Delay(500);
        //
        CMX869b_read(RxBuffer, sizeof(RxBuffer));
        HAL_Delay(500);
        //
        for (int i=0; i<N_RXBuf; i++) {
            xprintf("%02X ", RxBuffer[i]);
        }
        xprintf("\r\n");
        nn++;
    }
}
