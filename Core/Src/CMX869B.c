//
// Created by kikuchi on 2026/01/12.
//

#include "stm32f3xx_hal.h"
#include "../Inc/CMX869B.h"

#include <string.h>

#include "main.h"

extern SPI_HandleTypeDef hspi1;
HAL_StatusTypeDef Status;
uint8_t TxBuffer[3];
uint8_t RxBuffer[3];

//---------------------------------------
// 補助関数(低レベル）
//---------------------------------------
int spi_tx(const uint8_t len) {
    HAL_GPIO_WritePin(MODEM_CS_GPIO_Port, MODEM_CS_Pin, GPIO_PIN_RESET);
    Status = HAL_SPI_Transmit(&hspi1, TxBuffer, len, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(MODEM_CS_GPIO_Port, MODEM_CS_Pin, GPIO_PIN_SET);
    return Status;
}

int spi_rx(const uint8_t len) {
    HAL_GPIO_WritePin(MODEM_CS_GPIO_Port, MODEM_CS_Pin, GPIO_PIN_RESET);
    Status = HAL_SPI_TransmitReceive(&hspi1, TxBuffer, RxBuffer, len, HAL_MAX_DELAY);
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
    memset(RxBuffer, 0, 3);
    spi_tx(2);
    return Status;
}

int Receive_Status(uint8_t addr) {
    TxBuffer[0] = addr;
    memset(RxBuffer, 0, 3);
    spi_rx(3);
    return Status;
}

int Receive_Data(uint8_t addr) {
    TxBuffer[0] = RxData_ADDR;
    memset(RxBuffer, 0, 3);
    spi_rx(3);
    return Status;
}

//---------------------------------------
// 初期化
//---------------------------------------
void CMX869B_Init(void) {
    static CMX869B_GRE_TypeDef GRE = {0};
    static CMX869B_TxReg_TypeDef TxReg = {0};
    static CMX869B_RxReg_TypeDef RxReg = {0};
    static CMX869B_QamReg_TypeDef QamReg = {0};
    static CMX869B_QamStatus_TypeDef QamStatus = {0};

    //Reset
    Send_General_Reset();
    HAL_Delay(100);

    //GRE
    GRE.Bits.LB = 1;
    GRE.Bits.Pwr = 1;
    GRE.Bits.IrqEna = 1;
    GRE.Bits.IrqMask = 0b111111;
    Send_Cmd(GRE_ADDR, (uint8_t *)&GRE);

    //TxReg
    TxReg.Bits.TxLevel = 0b111;
    Send_Cmd(TxData_ADDR, (uint8_t *)&TxReg);

    //RxReg
    Send_Cmd(RxData_ADDR, (uint8_t *)&RxReg);

    //QamReg
    QamReg.Bits.command = 0b010;
    QamReg.Bits.protocol = 0b111;
    Send_Cmd(QamReg_ADDR, (uint8_t *)&QamReg);
}

//---------------------------------------
// 送信
// 割込の中でDMAを使ってすぐ戻る
//---------------------------------------

//---------------------------------------
// 受信
// 割込にすると9600bpsで約1msecごとになる。
//---------------------------------------
