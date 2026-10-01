//
// Created by kikuchi on 2026/01/12.
//

#include "stm32f3xx_hal.h"
#include "CMX869B.h"
#include <string.h>
#include "main.h"
#include "xprintf.h"


extern UART_HandleTypeDef huart2;
extern TIM_HandleTypeDef htim2;

CMX869B_GRE_TypeDef GRE = {0};
CMX869B_TxReg_TypeDef TxReg = {0};
CMX869B_RxReg_TypeDef RxReg = {0};
CMX869B_QamReg_TypeDef QamReg = {0};
CMX869B_StatusReg_TypeDef StatusReg = {0};
CMX869B_QamStatusReg_TypeDef QamStatusReg = {0};
//
extern int MODEM_MODE_GSE;

void cbux_global_reset() {
    //HAL_GPIO_WritePin(MODEM_CS_GPIO_Port, MODEM_CS_Pin, GPIO_PIN_RESET);
    for (int i = 0; i < 2; i++) {

    }
}


//---------------------------------------
// len 受信バイト(1 or 2)
// tx_data[0]はアドレス
//---------------------------------------
int spi_rx(const uint8_t len, uint8_t rx_data[]) {
    HAL_StatusTypeDef st;
    //CS=0
    HAL_GPIO_WritePin(C_CS_GPIO_Port, C_CS_Pin, GPIO_PIN_RESET);

    //CS=1
    HAL_GPIO_WritePin(C_CS_GPIO_Port, C_CS_Pin, GPIO_PIN_SET);
    return 0;
}

//---------------------------------------
// 補助関数(高レベル）
//---------------------------------------
int send_cmd(uint8_t addr, uint8_t Bytes[]) {
    uint8_t buffer[] = {addr, Bytes[1], Bytes[0]};
    return 0;
}

int receive_status(CMX869B_StatusReg_TypeDef *st) {
    uint8_t buffer[] = {StatusReg_ADDR, 0xFF, 0xFF};
    st->Bytes[0] = buffer[2];
    st->Bytes[1] = buffer[1];
    return 0;
}

int receive_qam_status(CMX869B_QamStatusReg_TypeDef *st) {
    uint8_t buffer[] = {QamStatusReg_ADDR, 0xFF, 0xFF};
    st->Bytes[0] = buffer[2];
    st->Bytes[1] = buffer[1];
    return 0;
}

int send_data(const uint8_t data) {
    uint8_t buffer[] = {TxData_ADDR, data};
    return 0;
}

int receive_data(uint8_t *st) {
    uint8_t buffer[] = {RxData_ADDR, 0xFF};
    *st = buffer[1];
    return 0;
}


void CMX869B_Init(void) {
    //Reset
    uint8_t rx_data;
    uint8_t dummy[] = {0, 0};

    for (int i = 0; i < 2; i++) {
    }

    // コマンドを書き込む（受信は無視）
    volatile int nc, tic;
    for (nc=0; nc<100000; nc++) {
        while (!(SPI1->SR & SPI_SR_TXE)){};
        HAL_GPIO_WritePin(CPU_MON_GPIO_Port, CPU_MON_Pin, GPIO_PIN_SET);
        SPI1->DR = 0x44;
        while (!(SPI1->SR & SPI_SR_TXE)){};
        HAL_GPIO_WritePin(CPU_MON_GPIO_Port, CPU_MON_Pin, GPIO_PIN_RESET);
        nn:;
    }

    // グローバルリセット
    send_cmd(General_Reset, dummy);

    //バス動作確認。Ring DetectがLOWだと1, Highだと0が返る
    receive_status(&StatusReg);

    // リセットビットでリセット
    GRE.Bits.Rst = 1;
    send_cmd(GRE_ADDR, GRE.Bytes);
    //HAL_Delay(1);

    //send_cmdで、うまくいくと22pinが発振する
    GRE.Bits.Pwr = 1;
    GRE.Bits.HighGain = 1;
    GRE.Bits.PatDet = 1;
    GRE.Bits.LB = 0;
    //GRE.Bits.LB = 1;
    GRE.Bits.Rst = 0;
    GRE.Bits.IrqEna = 1;
    GRE.Bits.IrqMask = 0b000000;
    send_cmd(GRE_ADDR, GRE.Bytes);
    receive_status(&StatusReg);
    //HAL_Delay(1);

    // モデム送受信の設定
    set_bell();
    //set_v22_ans();
    //set_v22_call();
    //set_v22_loop();
    //set_qam_answer();
    //set_qam_call();

    //RxDataにゴミが入っているので除去して割込許可
    receive_data(&rx_data);
    GRE.Bits.IrqMask = 0b000001;
    send_cmd(GRE_ADDR, GRE.Bytes);
    //HAL_Delay(1);
}

//-----------------------------------
//周期タイマで送信
//-----------------------------------
void StartvTxTask(void *argument) {
    int count = 0;
    uint8_t tx_data = 0;

    //送信タイマースタート
    HAL_TIM_Base_Start_IT(&htim2);

    //ループ
    for (;;) {
        HAL_GPIO_WritePin(CPU_MON_GPIO_Port, CPU_MON_Pin, GPIO_PIN_SET);
        receive_status(&StatusReg);
        if (StatusReg.Bits.TxDataReady == 1) {
            tx_data = '0' + (count++) % 10;
            send_data(tx_data);
        }
        HAL_GPIO_WritePin(CPU_MON_GPIO_Port, CPU_MON_Pin, GPIO_PIN_RESET);
    }
}

//-----------------------------------
//割込ハンドラで受信
//-----------------------------------
void StartvRxTask(void *argument) {
    static uint8_t rx_data = 'A';
    for (;;) {
        //受信割込待機
        //受信
        receive_status(&StatusReg);
        if (StatusReg.Bits.RxDataReady == 1) {
            receive_data(&rx_data);
        }
    }
}

void spi_putc(unsigned char c) {
    // 送信バッファが空（TXE: Transmit buffer empty）になるのを待つ
    while (!(SPI1->SR & SPI_SR_TXE)) {
        // 必要に応じて無限ループ防止用のカウンターなどを追加
    }
    // 送信データレジスタに書き込む
    SPI1->DR = c;
}
