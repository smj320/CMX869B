//
// Created by kikuchi on 25/02/14.
//
#include "main.h"
#include "cmx869b.h"

extern SPI_HandleTypeDef hspi1;

HAL_StatusTypeDef reg_write(uint8_t reg, uint8_t data1, uint8_t data2, int nByte);

HAL_StatusTypeDef reg_read(uint8_t reg, uint8_t *data1, uint8_t *data2, int nByte);

int n_cmd = 5;
uint8_t cmd_call[][3] = {
    {0x01, 0x00, 0x00},
    {0xE0, 0xA3, 0x00},
    {0xE1, 0xFE, 0x76},
    {0xE2, 0xFE, 0xF7},
    {0xEA, 0x00, 0x37},
};
uint8_t cmd_ans[][3] = {
    {0x01, 0x00, 0x00},
    {0xE0, 0xA3, 0x00},
    {0xE1, 0xFE, 0x76},
    {0xE2, 0xFE, 0xF7},
    {0xEA, 0x00, 0x3F},
};

void CMX869B_Init() {
    // 地上系は、ジャンパありでCali
    // Drillは、ジャンパなしなでAns
    static HAL_StatusTypeDef rc;
    static uint8_t data1, data2;
    static uint8_t RxData[2];
    static int is_rdy;
    int isGse = CMX869B_is_gse();

    if (isGse) {
        //CALL コマンド送出
        for (int i = 0; i < n_cmd; i++) {
            rc = reg_write(cmd_call[i][0], cmd_call[i][1], cmd_call[i][2], 2);
            rc = reg_read(0xE6, &RxData[0], &RxData[1], 2);
            rc = reg_read(0xEB, &RxData[0], &RxData[1], 2);
            HAL_Delay(1);
        }
        //ハンドシェーク待機
        do {
            is_rdy = 0;
            rc = reg_read(0xEB, &RxData[0], &RxData[1], 1);
            is_rdy = 1;
            HAL_Delay(1);
        } while (is_rdy == 1);
    } else {
        //ANS コマンド送出
        for (int i = 0; i < n_cmd; i++) {
            rc = reg_write(cmd_ans[i][0], cmd_ans[i][1], cmd_ans[i][2], 2);
            rc = reg_read(0xE6, &RxData[0], &RxData[1], 2);
            rc = reg_read(0xEB, &RxData[0], &RxData[1], 2);
            HAL_Delay(1);
        }
        //ハンドシェーク待機
    }
}

/**
 * SPIレジスタ書き込み
 * @param reg register address
 * @param data uint8_t[2]
 * @param nByte 1(8bit), or 2(16bit)
 * @return
 */
HAL_StatusTypeDef reg_write(uint8_t reg, uint8_t data1, uint8_t data2, int nByte) {
    static uint8_t pTxData[3];
    pTxData[0] = reg;
    pTxData[1] = data1;
    pTxData[2] = data2;
    HAL_GPIO_WritePin(MODEM_CS_GPIO_Port, MODEM_CS_Pin, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_StatusTypeDef rc = HAL_SPI_Transmit(&hspi1, pTxData, nByte + 1, 1000);
    HAL_Delay(1);
    HAL_GPIO_WritePin(MODEM_CS_GPIO_Port, MODEM_CS_Pin, GPIO_PIN_SET);
    return (rc);
}

/**
 * SPIレジスタ読み出し
 * @param reg resgster address
 * @param data uint8_t[2]
 * @param nByte 1(8bit) or 2(16bit)
 * @return
 */
HAL_StatusTypeDef reg_read(uint8_t reg, uint8_t *data1, uint8_t *data2, int nByte) {
    static uint8_t pTxData[3];
    static uint8_t pRxData[3];
    pTxData[0] = reg;
    pTxData[1] = 0x00;
    pTxData[2] = 0x00;
    HAL_GPIO_WritePin(MODEM_CS_GPIO_Port, MODEM_CS_Pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef rc = HAL_SPI_TransmitReceive(&hspi1, pTxData, pRxData, nByte + 1, 1000);
    HAL_GPIO_WritePin(MODEM_CS_GPIO_Port, MODEM_CS_Pin, GPIO_PIN_SET);
    if (nByte == 1) {
        *data1 = pRxData[1];
        return (rc);
    }
    *data1 = pRxData[1];
    *data2 = pRxData[2];
    return (rc);
}

int CMX869B_is_gse() {
    if (HAL_GPIO_ReadPin(MODEM_MODE_GPIO_Port, MODEM_MODE_Pin) != GPIO_PIN_RESET) {
        //ドリル->ジャンパなし->SET->Answer
        return (0);
    } else {
        //地上->ジャンパあり->Calling
        return (1);
    }
}

int CMX869B_Receive(uint8_t *rx_data) {
    static HAL_StatusTypeDef rc;
    static int has_rx, has_sig, has_rxIrq;
    static uint8_t data1, data2;
    rc = reg_read(0xE6, &data1, &data2, 2);
    has_sig = ((data1 >> 2) & 0x01);
    has_rx = ((data2 >> 6) & 0x01);
    has_rxIrq = ((data2 >> 0) & 0x01);
    if (!has_sig) return (-1);
    if (!has_rx) return (-2);
    rc = reg_read(0xE5, &data1, &data2, 2);
    if (rc == HAL_OK) {
        *rx_data = data1;
        return (1);
    }
    return (-3);
}

int CMX869B_Transmit(uint8_t tx_data) {
    static HAL_StatusTypeDef rc;
    int tx_rdy;
    int cnt = 0;
    uint8_t data1, data2;
    rc = reg_read(0xE6, &data1, &data2, 2);
    tx_rdy = ((data1 >> 4) & 0x01);
    cnt = 0;
    while (tx_rdy != 1) {
        if (cnt++ > 1000) break;
    }
    if (tx_rdy == 1) {
        rc = reg_write(0xE3, tx_data, 0, 1);
        return (0);
    }
    return (-1);
}
