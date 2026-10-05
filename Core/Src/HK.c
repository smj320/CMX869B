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

//
//********************************************
// データ送受信
//********************************************
void HKLoop(int is_gse) {
    static uint8_t flame[N_TX_BUFFER] = {0};
    uint8_t uart_rx_data;

    //GSEの場合はUARTを監視して、データがくればモデムになげる
    //Bitrateはパソコンのほうが早いので、送信に当たっては
    //文字の送出間隔を調整のこと
    if (is_gse==1) {
        for (;;) {
            HAL_UART_Receive(&huart2, &uart_rx_data,1,HAL_MAX_DELAY);
            CBUS_DATA_WRITE(uart_rx_data);
        }
    }

    //Drillの場合はフレーム転送モード
    for (;;) {
        for (int i=0;i<N_TX_BUFFER;i++) {
            flame[i]='0'+i%10;
        }
        CMX869B_write_buffered(flame,N_TX_BUFFER);
        HAL_Delay(1000);
    }
}
