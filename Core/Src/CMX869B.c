//
// Created by kikuchi on 2026/01/12.
//

#include "stm32f3xx_hal.h"
#include "../Inc/CMX869B.h"

#include <string.h>

#include "main.h"

extern SPI_HandleTypeDef hspi1;
HAL_StatusTypeDef Status;
#define SPI_BUFFER_SIZE 3
uint8_t TxBuffer[SPI_BUFFER_SIZE];
uint8_t RxBuffer[SPI_BUFFER_SIZE];

static CMX869B_GRE_TypeDef GRE = {0};
static CMX869B_TxReg_TypeDef TxReg = {0};
static CMX869B_RxReg_TypeDef RxReg = {0};
static CMX869B_QamReg_TypeDef QamReg = {0};
//
static CMX869B_StatusReg_TypeDef StatusReg = {0};
static CMX869B_QamStatusReg_TypeDef QamStatusReg = {0};

//---------------------------------------
// 補助関数(低レベル）
// 送受信バッファはグローバル
//---------------------------------------
int spi_tx(const uint8_t len) {
    HAL_GPIO_WritePin(MODEM_CS_GPIO_Port, MODEM_CS_Pin, GPIO_PIN_RESET);
    Status = HAL_SPI_Transmit(&hspi1, TxBuffer, len, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(MODEM_CS_GPIO_Port, MODEM_CS_Pin, GPIO_PIN_SET);
    return Status;
}

int spi_rx(const uint8_t len) {
    HAL_GPIO_WritePin(MODEM_CS_GPIO_Port, MODEM_CS_Pin, GPIO_PIN_RESET);
    Status = HAL_SPI_Transmit(&hspi1, TxBuffer, 1, HAL_MAX_DELAY);
    Status = HAL_SPI_Receive(&hspi1, RxBuffer, len, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(MODEM_CS_GPIO_Port, MODEM_CS_Pin, GPIO_PIN_SET);
    return Status;
}

//---------------------------------------
// 補助関数(高レベル）
//---------------------------------------
int Send_General_Reset() {
    TxBuffer[0] = General_Reset;
    spi_tx(1);
    return Status;
}

int Send_Cmd(const uint8_t addr, const uint8_t Bytes[]) {
    TxBuffer[0] = addr;
    TxBuffer[1] = Bytes[1];
    TxBuffer[2] = Bytes[0];
    spi_tx(3);
    return Status;
}

int Send_Data(const uint8_t data) {
    TxBuffer[0] = TxData_ADDR;
    TxBuffer[1] = data;
    spi_tx(2);
    return Status;
}

int Receive_Status(CMX869B_StatusReg_TypeDef *st) {
    memset(TxBuffer, 0, SPI_BUFFER_SIZE);
    memset(RxBuffer, 0, SPI_BUFFER_SIZE);
    TxBuffer[0] = StatusReg_ADDR;
    Status = spi_rx(2);
    st->Bytes[0] = RxBuffer[1];
    st->Bytes[1] = RxBuffer[0];
    return Status;
}

int Receive_Data(uint8_t *addr) {
    memset(TxBuffer, 0, SPI_BUFFER_SIZE);
    memset(RxBuffer, 0, SPI_BUFFER_SIZE);
    TxBuffer[0] = RxData_ADDR;
    memset(RxBuffer, 0, 3);
    spi_rx(1);
    *addr = RxBuffer[0];
    return Status;
}

int Receive_QamStatus(CMX869B_QamStatusReg_TypeDef *st) {
    memset(TxBuffer, 0, SPI_BUFFER_SIZE);
    memset(RxBuffer, 0, SPI_BUFFER_SIZE);
    TxBuffer[0] = StatusReg_ADDR;
    Status = spi_rx(2);
    st->Bytes[0] = RxBuffer[1];
    st->Bytes[1] = RxBuffer[0];
    return Status;
}

//---------------------------------------
// 初期化
// 地上側受信　モデムの受信割込をUARTにながす
// 地上側送信  UARTの受信割込でCMXにデータを流す
//---------------------------------------
void CMX869B_Init(void) {
    //Reset
    Send_General_Reset();

    //Receive Status
    //Ring DetectがLOWだと1, Highだと0が返る
    Receive_Status(&StatusReg);

    //レジスタリセット
    GRE.Bits.Rst = 1;
    Send_Cmd(GRE_ADDR, GRE.Bytes);

    //Send GRE, うまくいくと22pinが発振する
    GRE.Bits.Pwr = 1;
    GRE.Bits.HighGain = 1;
    GRE.Bits.PatDet = 1;
    GRE.Bits.LB = 1;
    GRE.Bits.Rst = 0;
    GRE.Bits.IrqMask = 0b100001;
    Send_Cmd(GRE_ADDR, GRE.Bytes);

    //Send TxReg
    //TxReg.Bits.TxMode = TxReg_Mode_V22_CALL;
    TxReg.Bits.TxMode = TxReg_Mode_BELL;
    TxReg.Bits.StartStop = 0b10; //Start-stop, NonParity
    TxReg.Bits.DataBits = 0b110; //8bit Stop1
    Send_Cmd(TxReg_ADDR, TxReg.Bytes);

    //Send RxReg
    //RxReg.Bits.RxMode = RxReg_Mode_V22_ANS;
    //RxReg.Bits.RxMode = RxReg_Mode_V22_CALL;
    RxReg.Bits.RxMode = RxReg_Mode_BELL;
    RxReg.Bits.StartStop_Synch = 0b110; //Start-stop, NonOverSpeed
    RxReg.Bits.BitsParity = 0b111; //8bit, NonParity
    Send_Cmd(RxReg_ADDR, RxReg.Bytes);

    //テスト送信
    uint8_t data = 0;
    for (uint16_t i = 0; i < 2000; i++) {
        Receive_Status(&StatusReg);
        Send_Data(i & 0xFF);
        Receive_Status(&StatusReg);
    }
    Receive_Status(&StatusReg);
}

//---------------------------------------
// 送信
// 割込の中でDMAを使ってすぐ戻る
//---------------------------------------

//---------------------------------------
// 受信
// 割込にすると9600bpsで約1msecごとになる。
//---------------------------------------
