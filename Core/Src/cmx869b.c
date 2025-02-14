//
// Created by kikuchi on 25/02/14.
//
#include "main.h"

extern SPI_HandleTypeDef hspi1;

HAL_StatusTypeDef reg_write(uint8_t reg, uint8_t data1, uint8_t data2) {
    static uint8_t pTxData[3];
    pTxData[0] = reg;
    pTxData[1] = data1;
    pTxData[2] = data2;
    HAL_GPIO_WritePin(MODEM_CS_GPIO_Port, MODEM_CS_Pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef rc = HAL_SPI_Transmit(&hspi1, pTxData, 3, 1000);
    HAL_GPIO_WritePin(MODEM_CS_GPIO_Port, MODEM_CS_Pin, GPIO_PIN_SET);
    return (rc);
}

HAL_StatusTypeDef reg_read(uint8_t reg, uint16_t *data) {
    static uint8_t pTxData[3];
    static uint8_t pRxData[3];
    pTxData[0] = reg;
    pTxData[1] = 0x00;
    pTxData[2] = 0x00;
    HAL_GPIO_WritePin(MODEM_CS_GPIO_Port, MODEM_CS_Pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef rc = HAL_SPI_TransmitReceive(&hspi1, pTxData, pRxData, 3, 1000);
    HAL_GPIO_WritePin(MODEM_CS_GPIO_Port, MODEM_CS_Pin, GPIO_PIN_SET);
    *data = pRxData[1]<<8 | pRxData[2];
    return (rc);
}

int cmx869b_init_Call() {
    static HAL_StatusTypeDef rc;
    uint16_t data;
    rc = reg_write(0x01, 0x00, 0x00);
    rc = reg_write(0xE0, 0x01, 0x40);
    rc = reg_write(0xE1, 0xF0, 0x16);
    rc = reg_write(0xE3, 0xF0, 0x36);
    rc = reg_write(0xEA, 0x00, 0x1F);
    rc = reg_read(0xEB, &data);
    rc = reg_write(0xE4, 0x00, 0x42);
    rc = reg_read(0xE5, &data);
    return (0);
}

int cmx869b_init_Answer() {
    static HAL_StatusTypeDef rc;
    uint16_t data;
    rc = reg_write(0x01, 0x00, 0x00);
    rc = reg_write(0xE0, 0x01, 0x40);
    rc = reg_write(0xE1, 0xF0, 0x16);
    rc = reg_write(0xE3, 0xF0, 0x36);
    rc = reg_write(0xEA, 0x00, 0x1F);
    rc = reg_read(0xEB, &data);
    rc = reg_write(0xE4, 0x00, 0x42);
    rc = reg_read(0xE5, &data);
    return (0);
}

void CMX869B_Init() {
    // ジャンパがなければPullUpでSETになる。
    // DrillはジャンパなしなのでSETになり、Answerになる。
    if (HAL_GPIO_ReadPin(MODEM_MODE_GPIO_Port, MODEM_MODE_Pin) != GPIO_PIN_RESET) {
        //ドリル->ジャンパなし->SET->Answer
        cmx869b_init_Answer();
    } else {
        //地上->ジャンパあり->Calling
        cmx869b_init_Call();
    }
}
