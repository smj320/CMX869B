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
        tx_data[0] = nn%10;
        xprintf("TX:%03d\r\n", tx_data[0]);
        CMX869B_write_data(tx_data,CMX_TX_BUFFER);
        HAL_Delay(1000);
        nn++;
    }
}
