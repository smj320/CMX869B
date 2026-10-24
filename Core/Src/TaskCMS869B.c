//
// Created by kikuchi on 2026/10/21.
//
#include <string.h>
#include "main.h"
#include "cmsis_os.h"
#include "task.h"
#include "stm32f3xx_hal.h"
#include "xprintf.h"
#include "stream_buffer.h"
#include "TaskCMX869B.h"
#include "TaskCMX869B_priv.h"

GREType GRE = {0};
StatusRegType StatusReg = {0};
TxRegType TxReg = {0};
RxRegType RxReg = {0};
QamRegType QamReg = {0};
QamStatusRegType QamStatusReg = {0};

StreamBufferHandle_t xStreamBufferTx;
StreamBufferHandle_t xStreamBufferRx;

// ****************************************************
// Externで外部に出す関数
// ****************************************************
// タスクの実体
void TaskCMX869BEntry(void *argument) {
    static int ptr = 0;
    static uint8_t tx_data = 0x55, rx_data = 0;
    //ジャンパークローズ(0)の場合はGSE
    int is_gse = HAL_GPIO_ReadPin(MODEM_MODE_GPIO_Port, MODEM_MODE_Pin) == 0 ? 1 : 0;
    xStreamBufferRx = xStreamBufferCreate(10, 1);
    xStreamBufferTx = xStreamBufferCreate(10, 1);
    cmx_init(is_gse);
    //割込の後始末
    for (;;) {
        //xprintf("Frank\r\n");
        //cbus_write_data(tx_data, 0);
        //rx_data = cbus_read_data(0);

        osDelay(1000);
    }
}
// 割込処理
volatile static int TxPtr = 0;
void INT_CMX869B_ISR() {
    static BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    static uint8_t tx_data, rx_data;
    GPIO_PinState pinState;
    uint8_t TxBuf[10], RxBuf[10], RxPtr = 0;
    //
    HAL_GPIO_WritePin(CPU_MON_GPIO_Port, CPU_MON_Pin, GPIO_PIN_SET);
    do {
        StatusReg.Word = cbus_read_reg(StatusReg_ADDR, 1);
        if (StatusReg.Bits.RxDataReady == 1) {
            rx_data = cbus_read_data(1);
            RxBuf[RxPtr++ % 10] = rx_data;
            xStreamBufferSendFromISR(
                xStreamBufferRx, &rx_data, 1, &xHigherPriorityTaskWoken);
        }
        if (StatusReg.Bits.TxDataReady == 1) {
            if (TxPtr < 10) {
                tx_data = TxPtr;
                cbus_write_data(tx_data, 1);
                TxBuf[TxPtr] = tx_data;
                TxPtr++;
            }
        }
        //割込線が戻っていなければ再度実行
        pinState = HAL_GPIO_ReadPin(C_INT_GPIO_Port,C_INT_Pin);
    } while (pinState == GPIO_PIN_RESET);
    HAL_GPIO_WritePin(CPU_MON_GPIO_Port, CPU_MON_Pin, GPIO_PIN_RESET);
}

// データ出力開始
void CMX869B_flush_Tx() {
    TxPtr = 0;
    cbus_write_data(0x03, 0);
    /*
    uint8_t c = 0;
    size_t res = xStreamBufferReceive(
        xStreamBufferTx, &c, sizeof(c), 0);
    if (res != 0) {
        cbus_write_data(c, 0);
    }
    */
}
// ****************************************************
// 分業
// ****************************************************
//-----------------
//初期化
//-----------------
void cmx_init(int is_gse) {
    static int is_loop_back = 1;
    static int is_Cal = 0;
    static uint8_t c = 0;

    // 動作モード選択
    is_loop_back = 1;
    is_Cal = 0;

    // SPIイネーブル
    __HAL_SPI_ENABLE(&hspi);

    // グローバルリセット
    cbus_write_reg(General_Reset_ADDR, 0, 0);

    // ステータス確認
    StatusReg.Word = cbus_read_reg(StatusReg_ADDR, 0);

    // Pwr=1,Rst=1で20ms以上保持して内部ロジックを確定させる(データシート GCR b8)
    // ここで水晶が発振する。
    GRE.Bits.Pwr = 1;
    GRE.Bits.Rst = 1;
    cbus_write_reg(GRE_ADDR, GRE.Word, 0);
    HAL_Delay(25);

    //send_cmdで、うまくいくと22pinが発振する
    //GRE.Bits.HighGain = 1;
    GRE.Bits.HighGain = 0;
    GRE.Bits.PatDet = 1;
    GRE.Bits.LB = is_loop_back == 1 ? 1 : 0;
    GRE.Bits.Rst = 0;
    GRE.Bits.IrqEna = 1;
    GRE.Bits.IrqMask = 0b000000;
    cbus_write_reg(GRE_ADDR, GRE.Word, 0);
    StatusReg.Word = cbus_read_reg(StatusReg_ADDR, 0);
    HAL_Delay(1);

    // モデム送受信の設定
    if (is_loop_back == 1) {
        set_bell();
        if (is_Cal == 1) {
            while (1);
        }
    } else {
        if (is_gse) {
            set_qam_call();
        } else {
            set_qam_answer();
        }
        //ハンドシェーク監視
        for (uint8_t tic = 0; tic < 60; tic++) {
            qam_monitor(is_gse, tic);
        }
    }
    StatusReg.Word = cbus_read_reg(StatusReg_ADDR, 0);
    c = cbus_read_data(0);
    StatusReg.Word = cbus_read_reg(StatusReg_ADDR, 0);
    GRE.Bits.IrqMask = 0b001001;
    cbus_write_reg(GRE_ADDR, GRE.Word, 0);
    StatusReg.Word = cbus_read_reg(StatusReg_ADDR, 0);
    xprintf("Initialized\r\n");
}

// ==========================================
// C-BUS操作関数
// ==========================================
void cbus_read_write(const uint8_t tx_data[], uint8_t rx_data[], int len, int is_isr) {
    //CS Low
    if (is_isr != 1) vTaskSuspendAll();
    C_CS_GPIO_Port->BSRR = C_CS_Pin << 16;
    // 2. 3バイト分をダイレクトに送受信する（例としてSPI1を使用）
    for (int i = 0; i < len; i++) {
        // 送信バッファが空になるのを待つ
        while (!(SPI1->SR & SPI_FLAG_TXE));
        // データを書き込む（書き込んだ瞬間にクロックが出始める）
        *(volatile uint8_t *) (&SPI1->DR) = tx_data[i];

        // 受信データが完了するのを待つ
        while (!(SPI1->SR & SPI_FLAG_RXNE));
        // 受信データを読み出す
        rx_data[i] = *(volatile uint8_t *) (&SPI1->DR);
    }
    //CS High
    C_CS_GPIO_Port->BSRR = C_CS_Pin;
    if (is_isr != 1) xTaskResumeAll();
}

void cbus_write_reg(uint8_t adr, uint16_t data, int is_isr) {
    static uint8_t tx_data[3], rx_data[3];
    tx_data[0] = adr;
    tx_data[1] = (data >> 8) & 0xFF;
    tx_data[2] = (data >> 0) & 0xFF;
    cbus_read_write(tx_data, rx_data, 3, is_isr);
}

uint16_t cbus_read_reg(uint8_t adr, int is_isr) {
    static uint8_t tx_data[3], rx_data[3];
    tx_data[0] = adr;
    tx_data[1] = tx_data[2] = 0x0;
    cbus_read_write(tx_data, rx_data, 3, is_isr);
    uint16_t result = (rx_data[1] << 8) + rx_data[2];
    return result;
}

void cbus_write_data(uint8_t data, int is_isr) {
    static uint8_t tx_data[2], rx_data[2];
    tx_data[0] = TxData_ADDR;
    tx_data[1] = data;
    cbus_read_write(tx_data, rx_data, 2, is_isr);
}

uint8_t cbus_read_data(int is_isr) {
    static uint8_t tx_data[2], rx_data[2];
    tx_data[0] = RxData_ADDR;
    tx_data[1] = 0x0;
    cbus_read_write(tx_data, rx_data, 2, is_isr);
    return rx_data[1];
}

// 割込要因のロギング
void int_log(int count) {
    xprintf("%04d:%d %d %d %d %d\r\n",
            count,
            StatusReg.Bits.IRQ,
            StatusReg.Bits.TxDataReady,
            StatusReg.Bits.TxDataUnderflow,
            StatusReg.Bits.RxDataReady,
            StatusReg.Bits.RxDataOverflow
    );
}

void qam_monitor(int is_gse, int tic) {
    QamStatusReg.Word = cbus_read_reg(QamStatusReg_ADDR, 0);
    xprintf("GSE:%d TI:%03d MSG:%02X SNR:%02X MODE:%02X\r\n",
            is_gse,
            tic, QamStatusReg.Bits.Messages,
            QamStatusReg.Bits.SNR,
            QamStatusReg.Bits.Mode);
    HAL_Delay(500);
}

// ==========================================
// モデムモード
// ==========================================
#define TX_LEVEL 0b111
#define RX_LEVEL 0b111

void set_bell() {
    // TX
    TxReg.Bits.TxMode = TxReg_Mode_BELL;
    TxReg.Bits.TxLevel = TX_LEVEL;
    TxReg.Bits.StartStop = 0b10; //Start-stop, NonParity
    TxReg.Bits.DataBits = 0b110; //8bit Stop1
    //TxReg.Bits.StartStop = 0b11; //Special Mode
    //TxReg.Bits.DataBits = 0b110; //HDLC Mode
    cbus_write_reg(TxReg_ADDR, TxReg.Word, 0);
    // RX
    RxReg.Bits.RxMode = RxReg_Mode_BELL;
    RxReg.Bits.RxLevel = RX_LEVEL;
    RxReg.Bits.StartStop_Synch = 0b110; //Start-stop, NonOverSpeed
    RxReg.Bits.BitsParity = 0b111; //8bit, NonParity
    //RxReg.Bits.StartStop_Synch = 0b111; //Special Mode
    //RxReg.Bits.BitsParity = 0b110; //HDLC Mode
    cbus_write_reg(RxReg_ADDR, RxReg.Word, 0);
}

// AutoModem ネゴシエーション必須
void set_qam_answer() {
    // TX
    TxReg.Bits.TxMode = TxReg_Mode_QAM_AUTO;
    TxReg.Bits.TxLevel = TX_LEVEL;
    TxReg.Bits.StartStop = 0b10; //Start-stop, NonParity
    TxReg.Bits.DataBits = 0b110; //8bit Stop1
    cbus_write_reg(TxReg_ADDR, TxReg.Word, 0);
    HAL_Delay(10);
    // RX
    RxReg.Bits.RxMode = RxReg_Mode_QAM_AUTO;
    RxReg.Bits.RxLevel = RX_LEVEL;
    RxReg.Bits.StartStop_Synch = 0b110; //Start-stop, NonOverSpeed
    RxReg.Bits.BitsParity = 0b111; //8bit, NonParity
    cbus_write_reg(RxReg_ADDR, RxReg.Word, 0);
    HAL_Delay(10);
    //QAM
    QamReg.Bits.br = QamMaxBR_14400;
    //QamReg.Bits.br = QamMaxBR_2400;
    QamReg.Bits.command = QamAnswer;
    QamReg.Bits.zeros = QamZeros;
    cbus_write_reg(QamCmdReg_ADDR, QamReg.Word, 0);
    HAL_Delay(10);
}

void set_qam_call() {
    // TX
    TxReg.Bits.TxMode = TxReg_Mode_QAM_AUTO;
    TxReg.Bits.TxLevel = TX_LEVEL;
    TxReg.Bits.StartStop = 0b10; //Start-stop, NonParity
    TxReg.Bits.DataBits = 0b110; //8bit Stop1
    cbus_write_reg(TxReg_ADDR, TxReg.Word, 0);
    HAL_Delay(10);
    // RX
    RxReg.Bits.RxMode = RxReg_Mode_QAM_AUTO;
    RxReg.Bits.RxLevel = RX_LEVEL;
    RxReg.Bits.StartStop_Synch = 0b110; //Start-stop, NonOverSpeed
    RxReg.Bits.BitsParity = 0b111; //8bit, NonParity
    cbus_write_reg(RxReg_ADDR, RxReg.Word, 0);
    HAL_Delay(10);
    //QAM
    QamReg.Bits.br = QamMaxBR_14400;
    //QamReg.Bits.br = QamMaxBR_2400;
    QamReg.Bits.command = QamCall;
    QamReg.Bits.zeros = QamZeros;
    cbus_write_reg(QamCmdReg_ADDR, QamReg.Word, 0);
    HAL_Delay(10);
}

// xprintf用
void uart_putc(unsigned char c) {
    while (!(huart.Instance->ISR & USART_ISR_TXE)) {
    }
    huart.Instance->TDR = c;
}
