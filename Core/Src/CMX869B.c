//
// Created by kikuchi on 2026/01/12.
//

#include "stm32f3xx_hal.h"
#include "../Inc/CMX869B.h"

#include <string.h>

#include "main.h"

extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart2;
HAL_StatusTypeDef Status;

#define SPI_BUFFER_SIZE 4
uint8_t SPI_Buffer[SPI_BUFFER_SIZE];
//
volatile uint8_t UART2_rxBuffer[1];
//
static CMX869B_GRE_TypeDef GRE = {0};
static CMX869B_TxReg_TypeDef TxReg = {0};
static CMX869B_RxReg_TypeDef RxReg = {0};
static CMX869B_QamReg_TypeDef QamReg = {0};
//
static CMX869B_StatusReg_TypeDef StatusReg = {0};
static CMX869B_QamStatusReg_TypeDef QamStatusReg = {0};

//---------------------------------------
// len 送信バイト(1 or 2)
// tx_data[0]はアドレス
//---------------------------------------
int spi_tx(const uint8_t len, const uint8_t tx_data[]) {

    // 1. CSをLOWにする
    HAL_GPIO_WritePin(MODEM_CS_GPIO_Port, MODEM_CS_Pin, GPIO_PIN_RESET);

    // SPI送信の実行
    if (HAL_SPI_Transmit(&hspi1, tx_data, len, 1) != HAL_OK)
    {
        // エラー処理
        Error_Handler();
    }
    HAL_GPIO_WritePin(MODEM_CS_GPIO_Port, MODEM_CS_Pin, GPIO_PIN_SET);
    return Status;
}

//---------------------------------------
// len 受信バイト(1 or 2)
// tx_data[0]はアドレス
//---------------------------------------
int spi_rx(const uint8_t len, uint8_t rx_data[]) {
    //CS=0
    HAL_GPIO_WritePin(MODEM_CS_GPIO_Port, MODEM_CS_Pin, GPIO_PIN_RESET);

    if (HAL_SPI_Receive(&hspi1, rx_data, len, 1) != HAL_OK)
    {
        // エラー処理
        Error_Handler();
    }
    //CS=1
    HAL_GPIO_WritePin(MODEM_CS_GPIO_Port, MODEM_CS_Pin, GPIO_PIN_SET);
    return Status;
}

//---------------------------------------
// 補助関数(高レベル）
//---------------------------------------
int send_cmd(const uint8_t addr, const uint8_t Bytes[]) {
    const uint8_t buffer[] = {addr, Bytes[1], Bytes[0]};
    if (addr == General_Reset) {
        spi_tx(1, buffer);
    }else {
        spi_tx(3, buffer);
    }
    return 0;
}

int receive_status(CMX869B_StatusReg_TypeDef *st) {
    uint8_t buffer[] = {StatusReg_ADDR, 0xFF, 0xFF};
    Status = spi_rx(3, buffer);
    st->Bytes[0] = buffer[2];
    st->Bytes[1] = buffer[1];
    return 0;
}

int send_data(const uint8_t data) {
    uint8_t buffer[] = {TxData_ADDR, data};
    Status = spi_tx(2, buffer);
    return 0;
}

int receive_data(uint8_t *st) {
    uint8_t buffer[] = {RxData_ADDR, 0xFF};
    Status = spi_rx(1, buffer);
    *st = buffer[1];
    return 0;
}
//---------------------------------------
// 初期化
// 地上側受信　モデムの受信割込をUARTにながす
// 地上側送信  UARTの受信割込でCMXにデータを流す
//---------------------------------------
// 1200bps 半二重
// 発振の確認、割込受信デバッグのループバックとかに使う
void set_bell(void) {
    // TX
    TxReg.Bits.TxMode = TxReg_Mode_BELL;
    TxReg.Bits.StartStop = 0b10; //Start-stop, NonParity
    TxReg.Bits.DataBits = 0b110; //8bit Stop1
    send_cmd(TxReg_ADDR, TxReg.Bytes);
    // RX
    RxReg.Bits.RxMode = RxReg_Mode_BELL;
    RxReg.Bits.StartStop_Synch = 0b110; //Start-stop, NonOverSpeed
    RxReg.Bits.BitsParity = 0b111; //8bit, NonParity
    send_cmd(RxReg_ADDR, RxReg.Bytes);
}
// 2400 bps 全二重
// ネゴシエーション不用なので、ブチ切りでもいける。
void set_v22_call(void) {
    // TX
    TxReg.Bits.TxMode = TxReg_Mode_V22_CALL;
    TxReg.Bits.StartStop = 0b10; //Start-stop, NonParity
    TxReg.Bits.DataBits = 0b110; //8bit Stop1
    send_cmd(TxReg_ADDR, TxReg.Bytes);
    // RX
    RxReg.Bits.RxMode = RxReg_Mode_V22_CALL;
    RxReg.Bits.StartStop_Synch = 0b110; //Start-stop, NonOverSpeed
    RxReg.Bits.BitsParity = 0b111; //8bit, NonParity
    send_cmd(RxReg_ADDR, RxReg.Bytes);
}

void set_v22_ans(void) {
    // TX
    TxReg.Bits.TxMode = TxReg_Mode_V22_ANS;
    TxReg.Bits.StartStop = 0b10; //Start-stop, NonParity
    TxReg.Bits.DataBits = 0b110; //8bit Stop1
    send_cmd(TxReg_ADDR, TxReg.Bytes);
    // RX
    RxReg.Bits.RxMode = RxReg_Mode_V22_ANS;
    RxReg.Bits.StartStop_Synch = 0b110; //Start-stop, NonOverSpeed
    RxReg.Bits.BitsParity = 0b111; //8bit, NonParity
    send_cmd(RxReg_ADDR, RxReg.Bytes);
}

// Auto modem
void set_auto_call(void) {
    // TX
    // RX
}

void set_autl_ans(void) {
    RxReg.Bits.RxMode = RxReg_Mode_V22_ANS;
}

void CMX869B_Init(void) {
    //Reset
    __HAL_SPI_ENABLE(&hspi1);
    const uint8_t dummy[] = {0,0};
    HAL_GPIO_WritePin(MODEM_CS_GPIO_Port, MODEM_CS_Pin, GPIO_PIN_SET);
    send_cmd(General_Reset,dummy);

    //Receive Status
    //Ring DetectがLOWだと1, Highだと0が返る
    receive_status(&StatusReg);

    // リセット
    GRE.Bits.Rst = 1;
    send_cmd(GRE_ADDR, GRE.Bytes);

    //Send GRE, うまくいくと22pinが発振する
    GRE.Bits.Pwr = 1;
    GRE.Bits.HighGain = 1;
    GRE.Bits.PatDet = 1;
    GRE.Bits.LB = 1;
    GRE.Bits.Rst = 0;
    GRE.Bits.IrqEna = 1;
    GRE.Bits.IrqMask = 0b000001;
    send_cmd(GRE_ADDR, GRE.Bytes);

    // モデム送受信の設定
    //set_bell();
    //set_v22_ans();
    set_v22_call();
    receive_status(&StatusReg);

    //

    const char *msg = "Hello, CMX869B!\n\r";
    HAL_UART_Transmit(&huart2, (uint8_t *) msg, strlen(msg), 1000);
}

//---------------------------------------
// UART2受信割込
// 初期化で割込を許可しておく
// HAL_UART_Receive_IT(&huart2, UART2_rxBuffer, 1);
//---------------------------------------
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART2) {
        const uint8_t rx_data = (uint8_t) (huart2.Instance->RDR);
        send_data(rx_data);
        // 次の1バイトを待機するために再度割り込みを有効化する
        HAL_UART_Receive_IT(&huart2, (uint8_t *) UART2_rxBuffer, 1);
    }
}

//---------------------------------------
// モデム受信割込
//---------------------------------------
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == MODEM_INT_Pin) // MODEM_INTピンからの割り込みか判定
    {
        //CS=0
        HAL_GPIO_WritePin(MODEM_CS_GPIO_Port, MODEM_CS_Pin, GPIO_PIN_RESET);
        //アドレス送信
        *(__IO uint8_t *)&hspi1.Instance->DR = RxData_ADDR;
        //受信クロック送信用ダミーデータ
        *(__IO uint8_t *)&hspi1.Instance->DR = 0xFF;
        //アドレス送信時のゴミ掃除
        (void)hspi1.Instance->DR;
        //SPIデータ取得
        uint8_t modem_data = *(__IO uint8_t *)&hspi1.Instance->DR;
        //CS=1
        HAL_GPIO_WritePin(MODEM_CS_GPIO_Port, MODEM_CS_Pin, GPIO_PIN_SET);
        //UARTに送信
        *(__IO uint8_t *)&huart2.Instance->TDR = modem_data;
    }
}
