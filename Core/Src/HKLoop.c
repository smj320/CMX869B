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
        for (int i=0; i<CMX_RX_BUFFER; i++) {
            tx_data[i] = nn%10;
        }
        // 送信
        xprintf("TX:%03d\r\n", tx_data[0]);
        CMX869B_write_data(tx_data,CMX_TX_BUFFER);
        HAL_Delay(1000);
        CMX869B_read_data(rx_data,CMX_RX_BUFFER);
        for (int i=0; i<CMX_RX_BUFFER; i++) {
            xprintf("%02X ", rx_data[i]);
        }
        xprintf("\r\n");
        HAL_Delay(5000);
        nn++;
    }
}
