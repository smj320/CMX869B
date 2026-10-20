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
        for (int i=0; i<N_TXBuf; i++) {
            TxBuffer[i] = 10*(nn%10)+i;
        }
        CMX869b_write(TxBuffer, sizeof(TxBuffer));
        HAL_Delay(500);
        //
        memset(RxBuffer, 0, sizeof(RxBuffer));
        int n = CMX869b_read(RxBuffer, sizeof(RxBuffer));
        HAL_Delay(5000);
        //
        //xprintf("\r\nLEN:%d ERR:%d  ", n, CMX869b_rx_errors());
        for (int i=0; i<n; i++) {
            xprintf("%02d ", RxBuffer[i]);
        }
        xprintf("\r\n");
        nn++;
    }
}
