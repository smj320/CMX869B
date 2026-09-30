//
// Created by kikuchi on 2026/01/12.
//

#include "stm32f3xx_hal.h"
#include "CMX869B.h"
#include <string.h>
#include "main.h"
#include "cmsis_os.h"
#include "task.h"
#include "xprintf.h"

extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart2;
extern TIM_HandleTypeDef htim2;
extern osMessageQueueId_t txQueueHandle;
extern osMessageQueueId_t rxQueueHandle;
extern osThreadId_t CMX869bTaskHandle;
HAL_StatusTypeDef Status;

#define SPI_BUFFER_SIZE 4
uint8_t SPI_Buffer[SPI_BUFFER_SIZE];
//
volatile uint8_t UART2_rxBuffer[1];
//
CMX869B_GRE_TypeDef GRE = {0};
CMX869B_TxReg_TypeDef TxReg = {0};
CMX869B_RxReg_TypeDef RxReg = {0};
CMX869B_QamReg_TypeDef QamReg = {0};
//
static CMX869B_StatusReg_TypeDef StatusReg = {0};
static CMX869B_QamStatusReg_TypeDef QamStatusReg = {0};
int Ptr = 80;
int N_ptr = 80;
//
extern int MODEM_MODE_GSE;
//---------------------------------------
// len 送信バイト(1 or 2)
// tx_data[0]はアドレス
//---------------------------------------
int spi_tx(uint8_t len, uint8_t tx_data[]) {
    // 1. CSをLOWにする
    vTaskSuspendAll();
    HAL_GPIO_WritePin(MODEM_CS_GPIO_Port, MODEM_CS_Pin, GPIO_PIN_RESET);

    // SPI送信の実行
    if ((Status = HAL_SPI_Transmit(&hspi1, tx_data, len, 10)) != HAL_OK) {
        // エラー処理
        Error_Handler();
    }
    HAL_GPIO_WritePin(MODEM_CS_GPIO_Port, MODEM_CS_Pin, GPIO_PIN_SET);
    xTaskResumeAll();
    return Status;
}

//---------------------------------------
// len 受信バイト(1 or 2)
// tx_data[0]はアドレス
//---------------------------------------
int spi_rx(const uint8_t len, uint8_t rx_data[]) {
    HAL_StatusTypeDef st;
    //CS=0
    vTaskSuspendAll();
    HAL_GPIO_WritePin(MODEM_CS_GPIO_Port, MODEM_CS_Pin, GPIO_PIN_RESET);

    if ((st = HAL_SPI_Receive(&hspi1, rx_data, len, 10)) != HAL_OK) {
        // エラー処理
        Error_Handler();
    }
    //CS=1
    HAL_GPIO_WritePin(MODEM_CS_GPIO_Port, MODEM_CS_Pin, GPIO_PIN_SET);
    xTaskResumeAll();
    return Status;
}

//---------------------------------------
// 補助関数(高レベル）
//---------------------------------------
int send_cmd(uint8_t addr, uint8_t Bytes[]) {
    uint8_t buffer[] = {addr, Bytes[1], Bytes[0]};
    if (addr == General_Reset) {
        spi_tx(1, buffer);
    } else {
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

int receive_gre(CMX869B_GRE_TypeDef *st) {
    uint8_t buffer[] = {GRE_ADDR, 0xFF, 0xFF};
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
    Status = spi_rx(2, buffer);
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

void set_v22_loop(void) {
    // TX
    TxReg.Bits.TxMode = TxReg_Mode_V22_ANS;
    TxReg.Bits.StartStop = 0b10; //Start-stop, NonParity
    TxReg.Bits.DataBits = 0b110; //8bit Stop1
    send_cmd(TxReg_ADDR, TxReg.Bytes);
    // RX
    RxReg.Bits.RxMode = RxReg_Mode_V22_CALL;
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
    // TX
    // RX
}

void CMX869B_Init(void) {
    //Reset
    uint8_t rx_data;
    uint8_t dummy[] = {0, 0};
    __HAL_SPI_ENABLE(&hspi1);

    // グローバルリセット
    send_cmd(General_Reset, dummy);

    //バス動作確認。Ring DetectがLOWだと1, Highだと0が返る
    receive_status(&StatusReg);

    // リセットビットでリセット
    GRE.Bits.Rst = 1;
    send_cmd(GRE_ADDR, GRE.Bytes);

    //send_cmdで、うまくいくと22pinが発振する
    GRE.Bits.Pwr = 1;
    GRE.Bits.HighGain = 1;
    GRE.Bits.PatDet = 1;
    //GRE.Bits.LB = 0;
    GRE.Bits.LB = 1;
    GRE.Bits.Rst = 0;
    GRE.Bits.IrqEna = 1;
    GRE.Bits.IrqMask = 0b000000;
    send_cmd(GRE_ADDR, GRE.Bytes);
    receive_status(&StatusReg);

    // モデム送受信の設定
    set_bell();
    //set_v22_ans();
    //set_v22_call();
    //set_v22_loop();

    //RxDataにゴミが入っているので除去して割込許可
    receive_data(&rx_data);
    GRE.Bits.IrqMask = 0b000001;
    send_cmd(GRE_ADDR, GRE.Bytes);

    //送信タイマースタート
    HAL_TIM_Base_Start_IT(&htim2);
}

//-----------------------------------
//タイマ割込で送信
//-----------------------------------
void StartvTxTask(void *argument) {
    int count = 0;
    uint8_t tx_data = 0;
    CMX869B_Init();

    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        HAL_GPIO_WritePin(CPU_MON_GPIO_Port, CPU_MON_Pin, GPIO_PIN_SET);
        receive_status(&StatusReg);
        if (StatusReg.Bits.TxDataReady==1) {
            tx_data = '0'+(count++)%10;
            send_data(tx_data);
        }
        HAL_GPIO_WritePin(CPU_MON_GPIO_Port, CPU_MON_Pin, GPIO_PIN_RESET);
    }
}

//-----------------------------------
//割込ハンドラで受信
//-----------------------------------
void StartvRxTask(void *argument) {
    uint8_t rx_data='A';
    for (;;) {
        //受信割込待機
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        //受信
        receive_status(&StatusReg);
        if (StatusReg.Bits.RxDataReady==1) {
            receive_data(&rx_data);
            xprintf("%c",rx_data);
        }
    }
}

//************************
// xprintf用
//************************
void uart_putc(unsigned char c) {
    // 送信データレジスタが空（TXE: Transmit Data Register Empty）になるのを待つ
    while (!(USART2->ISR & USART_ISR_TXE)) {
        // 必要に応じて無限ループ防止用のカウンターなどを追加
    }
    // 送信データレジスタに直接書き込む
    USART2->TDR = c;
}