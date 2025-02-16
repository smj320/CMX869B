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
/**
 * ５秒間20Hz(50msec)のRINGパルスを送る
 */
 void cmx869b_ring()
{
     int i;
     for(i=0;i<50;i++){
         HAL_GPIO_WritePin(MODEM_CS_GPIO_Port, MODEM_CS_Pin, GPIO_PIN_SET);
         HAL_Delay(25);
         HAL_GPIO_WritePin(MODEM_CS_GPIO_Port, MODEM_CS_Pin, GPIO_PIN_RESET);
         HAL_Delay(25);
     }
}

/**
 * こちらがキャリアを待機する。。地上側、D3のジャンパをクローズ。
 * @return
 */
int cmx869b_init_Call() {
    static HAL_StatusTypeDef rc;
    rc = reg_write(0x01, 0x00, 0x00);   //RESET
    rc = reg_write(0xE0, 0x21, 0x00);   //GRE
    cmx869b_ring();
    rc = reg_write(0xE1, 0xFE, 0x76);   //TX_REG
    rc = reg_write(0xE2, 0xFE, 0xF6);   //RX_REG
    rc = reg_write(0xEA, 0x00, 0x17);   //QAM_REG
    //ハンドシェーク完了を待機
    return (0);
}

/**
 * こちらがキャリアを出す。ドリル側、D3のジャンパをオープン。
 * @return
 */
int cmx869b_init_Answer() {
    static HAL_StatusTypeDef rc;
    rc = reg_write(0x01, 0x00, 0x00);   //RESET
    rc = reg_write(0xE0, 0x21, 0x00);   //GRE
    rc = reg_write(0xE1, 0xFE, 0x76);   //TX_REG
    rc = reg_write(0xE2, 0xFE, 0xF6);   //RX_REG
    rc = reg_write(0xEA, 0x00, 0x1F);   //QAM_REG
    //ハンドシェーク完了を待機
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
