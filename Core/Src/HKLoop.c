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
    uint8_t tx_data[CMX_TX_BUFFER];
    uint8_t rx_data[CMX_TX_BUFFER];
    int nn=0;

    CMX869B_Init(is_gse);

    //Drillの場合はフレーム転送モード
    for (;;) {
        // 送信
        xprintf("TX:%03d\r\n", nn++);
        CMX869B_write_data(rx_data,CMX_TX_BUFFER);
        HAL_Delay(100);
        nn++;
    }
}
