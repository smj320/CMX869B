//
// Created by kikuchi on 2026/09/27.
//
#include "string.h"
#include "main.h"
#include "HK.h"
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
        CMX869B_write(tx_data,CMX_TX_BUFFER);
        CMX869B_read(rx_data,CMX_TX_BUFFER);
        HAL_Delay(5000);
        nn++;
    }
}
