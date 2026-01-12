//
// Created by kikuchi on 2026/01/12.
//

#include "stm32f3xx_hal.h"
#include "../Inc/CMX869B.h"

extern SPI_HandleTypeDef hspi1;
HAL_StatusTypeDef Status;

int Write_Cmd(const uint8_t addr, const uint8_t Bytes[], uint16_t len) {
    uint8_t data[3];
    data[0] = addr;
    data[1] = Bytes[1];
    data[2] = Bytes[0];
    Status = HAL_SPI_Transmit(&hspi1, data, 3, HAL_MAX_DELAY);
    return Status;
}

int Read_Status(const uint8_t addr, const uint8_t Bytes[]) {
    uint8_t TxData[3] = {0};
    uint8_t RxData[3] = {0};
    TxData[0] = addr;
    Status = HAL_SPI_TransmitReceive(&hspi1, TxData, RxData, 3, HAL_MAX_DELAY);
    return Status;
}

void CMX869B_Init(void) {
    static CMX869B_GRE_TypeDef GRE = {0};
    static CMX869B_TxReg_TypeDef TxReg = {0};
    static CMX869B_RxReg_TypeDef RxReg = {0};
    static CMX869B_QamReg_TypeDef QamReg = {0};
    static CMX869B_QamStatus_TypeDef QamStatus = {0};

    //GRE
    GRE.Bits.LB = 1;
    GRE.Bits.Pwr = 1;
    GRE.Bits.IrqEna = 1;
    GRE.Bits.IrqMask = 0b111111;

    //TxReg
    TxReg.Bits.TxLevel = 0b111;

}