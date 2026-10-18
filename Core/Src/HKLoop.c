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
        for (int i=0;i<CMX_TX_BUFFER;i++) tx_data[i]=i;
        CMX869B_write_data(tx_data,CMX_TX_BUFFER);
        // 送信完了を待つ
        HAL_Delay(200);
        //　データ受け取り
        CMX869B_read_data(rx_data,CMX_TX_BUFFER);
        HAL_Delay(5000);
        nn++;
    }
}
