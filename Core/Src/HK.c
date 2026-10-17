//
// Created by kikuchi on 2026/09/27.
//
#include "string.h"
#include "main.h"
#include "CMX869b.h"
#include "HK.h"
#include "xprintf.h"
//
extern UART_HandleTypeDef huart2;
extern TIM_HandleTypeDef htim2;

extern uint8_t TX_buffer[];
extern uint8_t RX_buffer[];
extern int TX_ptr;
extern int RX_ptr;
extern CMX869B_StatusReg_TypeDef StatusReg;

//
//********************************************
// データ送受信
//********************************************
void HKLoop(int is_gse) {
    static uint8_t flame[N_TX_BUFFER] = {0};
    uint16_t tx_data, rx_data;

    //GSEの場合はUARTを監視して、データがくればモデムになげる
    //Bitrateはパソコンのほうが早いので、送信に当たっては
    //文字の送出間隔を調整のこと
    /*
    if (is_gse==1) {
        for (;;) {
            HAL_UART_Receive(&huart2, &uart_rx_data,1,HAL_MAX_DELAY);
            CBUS_DATA_WRITE(uart_rx_data);
        }
    }
    */
    //Drillの場合はフレーム転送モード
    int nn = 0;
    int nw = 20;
    CMX869B_StatusReg_TypeDef st[100];
    for (;;) {
        //送信後のステータスを見てみる
        xprintf("%04d--------\r\n", nn);
        xprintf("TX----\r\n", nn);
        TX_ptr=1;
        CBUS_ST_READ(&StatusReg.Word);
        CBUS_DATA_WRITE(TX_buffer[0]);
        //CBUS_ST_READ(&StatusReg.Word);
        /*
        xprintf("Status: %02d %02d %02d %02d\r\n",
            StatusReg.Bits.TxDataReady, StatusReg.Bits.TxDataUnderflow,
            StatusReg.Bits.RxDataReady, StatusReg.Bits.RxDataOverflow);
        CBUS_DATA_WRITE(TX_buffer[0]);
        */
        /*
        for (int i = 0; i < nw; i++) {
            //ステータスを読んでいるだけなので、割込線の挙動はいかに
            //読んでまた戻るか
            CBUS_ST_READ(&StatusReg.Word);
            st[i] = StatusReg;
            HAL_Delay(1);
        }
        for (int i = 0; i < nw; i++) {
            xprintf("%02d TxRdy:%d TxUnd:%d RxRdy:%d RxOvr:%d\r\n",
                tx_data,
                st[i].Bits.TxDataReady, st[i].Bits.TxDataUnderflow,
                st[i].Bits.RxDataReady, st[i].Bits.RxDataOverflow);
        }
        HAL_Delay(20);  //Bellの場合、データは10msecで送信出来ている
        xprintf("RX----\r\n", nn);
        rx_data= 0;
        CBUS_DATA_READ(&rx_data);
        for (int i = 0; i < 5; i++) {
            CBUS_ST_READ(&StatusReg.Word);
            st[i] = StatusReg;
            HAL_Delay(1);
        }
        for (int i = 0; i < 5; i++) {
            xprintf("%02d TxRdy:%d TxUnd:%d RxRdy:%d RxOvr:%d\r\n",
                rx_data,
                st[i].Bits.TxDataReady, st[i].Bits.TxDataUnderflow,
                st[i].Bits.RxDataReady, st[i].Bits.RxDataOverflow);
        }
        */
        HAL_Delay(5000);
        nn++;
    }
}
