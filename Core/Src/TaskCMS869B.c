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

int Is_gse = 0;

// ****************************************************
// Externで外部に出す関数
// ****************************************************
// タスクの実体
#define STREAM_BUFFER_SIZE_BYTES 20
#define CMX_POLL_MS 5
#define CMX_QAM_STATUS_MS 200
static TaskHandle_t hTaskCMX869B = NULL;
// 他タスク参照用のQAMステータスのコピー(16bitなので読み書きはアトミック)
static volatile uint16_t qam_status_word = 0;
void TaskCMX869BEntry(void *argument) {
    hTaskCMX869B = xTaskGetCurrentTaskHandle();
    // Stream Bufferの作成
    xStreamBufferTx = xStreamBufferCreate(STREAM_BUFFER_SIZE_BYTES,0);
    xStreamBufferRx = xStreamBufferCreate(STREAM_BUFFER_SIZE_BYTES,0);
    //ジャンパークローズ(0)の場合はGSE
    int Is_gse = HAL_GPIO_ReadPin(MODEM_MODE_GPIO_Port, MODEM_MODE_Pin) == 0 ? 1 : 0;

    cmx_init(Is_gse);
    // 割込(ISRからの通知)またはタイムアウトで起きてステータスを処理する
    // タイムアウトは割込の取りこぼし対策のポーリングを兼ねる
    // CMX869BへのSPIアクセスはこのタスクだけが行う
    TickType_t qam_last = xTaskGetTickCount();
    for (;;) {
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(CMX_POLL_MS));
        cmx_service();
        // QAMステータスを定期的に読んでおく(SNR/Modeの監視用)
        if (xTaskGetTickCount() - qam_last >= pdMS_TO_TICKS(CMX_QAM_STATUS_MS)) {
            qam_last = xTaskGetTickCount();
            QamStatusReg.Word = cbus_read_reg(QamStatusReg_ADDR);
            qam_status_word = QamStatusReg.Word;
        }
        HAL_GPIO_WritePin(CPU_MON_GPIO_Port, CPU_MON_Pin, GPIO_PIN_RESET);
    }
}
// QAMステータスの取得(TaskCMX869Bが定期的に読んだ値のコピーを返す。SPIアクセスはしない)
void CMX869B_get_qam_status(uint8_t *mode, uint8_t *snr, uint8_t *msg) {
    QamStatusRegType s;
    s.Word = qam_status_word;
    *mode = s.Bits.Mode;
    *snr = s.Bits.SNR;
    *msg = s.Bits.Messages;
}
// 送信バッファに詰めたデータの送信開始
// TXレジスタが空のままだとTxDataReadyの割込は来ないので、タスクを起こして書かせる
void CMX869B_flush_Tx() {
    if (hTaskCMX869B != NULL) xTaskNotifyGive(hTaskCMX869B);
}
// 1文字送信(送信バッファに詰めて送信開始)
void CMX869B_putc(uint8_t tx_data) {
    xStreamBufferSend(xStreamBufferTx, &tx_data, 1, pdMS_TO_TICKS(100));
    CMX869B_flush_Tx();
}
// 割込処理 (タスクに通知するだけ)
void INT_CMX869B_ISR() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    HAL_GPIO_WritePin(CPU_MON_GPIO_Port, CPU_MON_Pin, GPIO_PIN_SET);
    if (hTaskCMX869B != NULL) {
        vTaskNotifyGiveFromISR(hTaskCMX869B, &xHigherPriorityTaskWoken);
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
// ステータスを読んで送受信する。割込線が戻るまで繰り返す
#define CMX_SERVICE_MAX_LOOP 8
void cmx_service() {
    uint8_t tx_data, rx_data;
    for (int i = 0; i < CMX_SERVICE_MAX_LOOP; i++) {
        // 先にEXTIを消す。以降のエッジは保留として残り、次の通知になる
        __HAL_GPIO_EXTI_CLEAR_IT(C_INT_Pin);
        StatusReg.Word = cbus_read_reg(StatusReg_ADDR);
        // TXレジスタがReadyなら書く
        if (StatusReg.Bits.TxDataReady == 1) {
            if (xStreamBufferReceive(xStreamBufferTx, &tx_data, 1, 0) == 1) {
                cbus_write_data(tx_data);
            }
        }
        // RXレジスタがReadyなら読む
        if (StatusReg.Bits.RxDataReady == 1) {
            rx_data = cbus_read_data();
            xStreamBufferSend(xStreamBufferRx, &rx_data, 1, 0);
        }
        //割込線が戻っていれば終了
        if (HAL_GPIO_ReadPin(C_INT_GPIO_Port, C_INT_Pin) == GPIO_PIN_SET) break;
    }
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
    is_loop_back = 0;
    is_Cal = 0;

    // SPIイネーブル
    __HAL_SPI_ENABLE(&hspi);

    // グローバルリセット
    cbus_write_reg(General_Reset_ADDR, 0);

    // ステータス確認
    StatusReg.Word = cbus_read_reg(StatusReg_ADDR);

    // Pwr=1,Rst=1で20ms以上保持して内部ロジックを確定させる(データシート GCR b8)
    // ここで水晶が発振する。
    GRE.Bits.Pwr = 1;
    GRE.Bits.Rst = 1;
    cbus_write_reg(GRE_ADDR, GRE.Word);
    HAL_Delay(25);

    //send_cmdで、うまくいくと22pinが発振する
    //GRE.Bits.HighGain = 1;
    GRE.Bits.HighGain = 0;
    GRE.Bits.PatDet = 1;
    GRE.Bits.LB = is_loop_back == 1 ? 1 : 0;
    GRE.Bits.Rst = 0;
    GRE.Bits.IrqEna = 1;
    GRE.Bits.IrqMask = 0b000000;
    cbus_write_reg(GRE_ADDR, GRE.Word);
    StatusReg.Word = cbus_read_reg(StatusReg_ADDR);
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
    GRE.Bits.IrqMask = 0b001001;
    cbus_write_reg(GRE_ADDR, GRE.Word);
    xprintf("Initialized\r\n");
}

// ==========================================
// C-BUS操作関数
// ==========================================
// TaskCMX869Bからのみ呼ぶこと(ISR・他タスクからは呼ばない)
void cbus_read_write(const uint8_t tx_data[], uint8_t rx_data[], int len) {
    //CS Low
    taskENTER_CRITICAL();
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
    taskEXIT_CRITICAL();
}

void cbus_write_reg(uint8_t adr, uint16_t data) {
    uint8_t tx_data[3], rx_data[3];
    tx_data[0] = adr;
    tx_data[1] = (data >> 8) & 0xFF;
    tx_data[2] = (data >> 0) & 0xFF;
    cbus_read_write(tx_data, rx_data, 3);
}

uint16_t cbus_read_reg(uint8_t adr) {
    uint8_t tx_data[3], rx_data[3];
    tx_data[0] = adr;
    tx_data[1] = tx_data[2] = 0x0;
    cbus_read_write(tx_data, rx_data, 3);
    uint16_t result = (rx_data[1] << 8) + rx_data[2];
    return result;
}

void cbus_write_data(uint8_t data) {
    uint8_t tx_data[2], rx_data[2];
    tx_data[0] = TxData_ADDR;
    tx_data[1] = data;
    cbus_read_write(tx_data, rx_data, 2);
}

uint8_t cbus_read_data(void) {
    uint8_t tx_data[2], rx_data[2];
    tx_data[0] = RxData_ADDR;
    tx_data[1] = 0x0;
    cbus_read_write(tx_data, rx_data, 2);
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
    QamStatusReg.Word = cbus_read_reg(QamStatusReg_ADDR);
    xprintf("GSE:%d TI:%03d MSG:%02X SNR:%02X MODE:%02X\r\n",
            is_gse,
            tic, QamStatusReg.Bits.Messages,
            QamStatusReg.Bits.SNR,
            QamStatusReg.Bits.Mode);
    HAL_Delay(500);
}

// ==========================================
// モデムモード
// ==================================    uint8_t TX_LEVEL;
#define TX_LEVEL 0b101;
#define RX_LEVEL 0b111;

void set_bell() {
    // TX
    TxReg.Bits.TxMode = TxReg_Mode_BELL;
    TxReg.Bits.TxLevel = TX_LEVEL;
    TxReg.Bits.StartStop = 0b10; //Start-stop, NonParity
    TxReg.Bits.DataBits = 0b111; //8bit Stop2
    //TxReg.Bits.StartStop = 0b11; //Special Mode
    //TxReg.Bits.DataBits = 0b110; //HDLC Mode
    cbus_write_reg(TxReg_ADDR, TxReg.Word);
    // RX
    RxReg.Bits.RxMode = RxReg_Mode_BELL;
    RxReg.Bits.RxLevel = RX_LEVEL;
    RxReg.Bits.StartStop_Synch = 0b110; //Start-stop, NonOverSpeed
    RxReg.Bits.BitsParity = 0b110; //8bit, NonParity (0b111は8bit+Parity)
    //RxReg.Bits.StartStop_Synch = 0b111; //Special Mode
    //RxReg.Bits.BitsParity = 0b110; //HDLC Mode
    cbus_write_reg(RxReg_ADDR, RxReg.Word);
}

// AutoModem ネゴシエーション必須
void set_qam_answer() {
    // TX
    TxReg.Bits.TxMode = TxReg_Mode_QAM_AUTO;
    TxReg.Bits.TxLevel = TX_LEVEL;
    TxReg.Bits.StartStop = 0b10; //Start-stop, NonParity
    TxReg.Bits.DataBits = 0b111; //8bit Stop2 (Stop1で文字が隙間なく連続すると相手側で化けるため)
    cbus_write_reg(TxReg_ADDR, TxReg.Word);
    HAL_Delay(10);
    // RX
    RxReg.Bits.RxMode = RxReg_Mode_QAM_AUTO;
    RxReg.Bits.RxLevel = RX_LEVEL;
    RxReg.Bits.StartStop_Synch = 0b110; //Start-stop, NonOverSpeed
    RxReg.Bits.BitsParity = 0b110; //8bit, NonParity (0b111は8bit+Parity)
    cbus_write_reg(RxReg_ADDR, RxReg.Word);
    HAL_Delay(10);
    //QAM
    QamReg.Bits.br = QamMaxBR_14400;
    //QamReg.Bits.br = QamMaxBR_12000;
    //QamReg.Bits.br = QamMaxBR_9600;
    //QamReg.Bits.br = QamMaxBR_2400;
    QamReg.Bits.command = QamAnswer;
    QamReg.Bits.zeros = QamZeros;
    cbus_write_reg(QamCmdReg_ADDR, QamReg.Word);
    HAL_Delay(10);
}

void set_qam_call() {
    // TX
    TxReg.Bits.TxMode = TxReg_Mode_QAM_AUTO;
    TxReg.Bits.TxLevel = TX_LEVEL;
    TxReg.Bits.StartStop = 0b10; //Start-stop, NonParity
    TxReg.Bits.DataBits = 0b111; //8bit Stop2 (Stop1で文字が隙間なく連続すると相手側で化けるため)
    cbus_write_reg(TxReg_ADDR, TxReg.Word);
    HAL_Delay(10);
    // RX
    RxReg.Bits.RxMode = RxReg_Mode_QAM_AUTO;
    RxReg.Bits.RxLevel = RX_LEVEL;
    RxReg.Bits.StartStop_Synch = 0b110; //Start-stop, NonOverSpeed
    RxReg.Bits.BitsParity = 0b110; //8bit, NonParity (0b111は8bit+Parity)
    cbus_write_reg(RxReg_ADDR, RxReg.Word);
    HAL_Delay(10);
    //QAM
    QamReg.Bits.br = QamMaxBR_14400;
    //QamReg.Bits.br = QamMaxBR_12000;
    //QamReg.Bits.br = QamMaxBR_9600;
    //QamReg.Bits.br = QamMaxBR_2400;
    QamReg.Bits.command = QamCall;
    QamReg.Bits.zeros = QamZeros;
    cbus_write_reg(QamCmdReg_ADDR, QamReg.Word);
    HAL_Delay(10);
}

// xprintf用
void uart_putc(unsigned char c) {
    while (!(huart.Instance->ISR & USART_ISR_TXE)) {
    }
    huart.Instance->TDR = c;
}
