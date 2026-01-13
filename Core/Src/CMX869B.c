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
    TxBuffer[0] = data;
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
    //
    static CMX869B_StatusReg_TypeDef StatusReg = {0};

    //Reset
    Send_General_Reset();

    //Receive Status
    //Ring DetectがLOWだと1, Highだと0が返る
    Receive_Status(&StatusReg);

    //Send GRE
    //　GRE.Bits.Equ = 1;
    GRE.Bits.LB = 1;
    GRE.Bits.Pwr = 1;
    GRE.Bits.IrqEna = 1;
    GRE.Bits.IrqMask = 0b111111;
    Send_Cmd(GRE_ADDR, (uint8_t *)&GRE);

    //Receive Status
    Receive_Status(&StatusReg);

    //Send TxReg
    TxReg.Bits.TxLevel = 0b111;
    Send_Cmd(TxData_ADDR, (uint8_t *)&TxReg);

    //Send RxReg
    Send_Cmd(RxData_ADDR, (uint8_t *)&RxReg);

    //Send QamReg
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
