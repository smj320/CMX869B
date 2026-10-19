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
    uint8_t c;
    CMX869B_Status st;
    int nn=0;

    CMX869B_Init(is_gse);

    //Drillの場合はフレーム転送モード
    for (;;) {
        st = CMX869B_status();
        xprintf("%04d RX:%02d TX:%02d\r\n", nn, st.Bits.RxDataReady, st.Bits.TxDataReady);
        if (st.Bits.TxDataReady==1) {
            CMX869B_putc(nn%10);
        }
        if (st.Bits.RxDataReady==1) {
            c = CMX869B_getc();
            xprintf("RCV: %d\r\n", c);
        }
        HAL_Delay(1000);
        nn++;
    }
}
