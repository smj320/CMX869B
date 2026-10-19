//
// Created by kikuchi on 2026/01/12.
//

#include <string.h>
#include "stm32f3xx_hal.h"
#include "stm32f3xx_ll_spi.h"
#include "CMX869B.h"
#include "xprintf.h"
#include "SEGGER_RTT.h"

// ==========================================
// レジスタとビットフィールド
// ==========================================
// --------------
// General Reset
// --------------
#define General_Reset_ADDR 0x01
// --------------
// Global Register
// --------------
#define GRE_ADDR 0xE0

union {
    uint16_t Word;

    struct {
        uint16_t IrqMask: 6; //b0
        uint16_t IrqEna: 1;
        uint16_t Rst: 1;
        uint16_t Pwr: 1;
        uint16_t RlyDrv: 1;
        uint16_t _2C: 1;
        uint16_t LB: 1;
        uint16_t Zero: 1;
        uint16_t PatDet: 1;
        uint16_t HighGain: 1;
        uint16_t Equ: 1;
    } __attribute__((packed)) Bits;
} GRE = {0};

// --------------
// Tx Register
// --------------
#define TxReg_ADDR 0xE1
#define TxReg_Mode_QAM_AUTO 0b1111
#define TxReg_Mode_V22_ANS 0b1011
#define TxReg_Mode_V22_CALL 0b1010
#define TxReg_Mode_BELL 0b0011

union {
    uint16_t Word;

    struct {
        uint16_t DataBits: 3; //b0
        uint16_t StartStop: 2;
        uint16_t Scramble: 2;
        uint16_t GardTone: 2;
        uint16_t TxLevel: 3;
        uint16_t TxMode: 4;
    } __attribute__((packed)) Bits;
} TxReg = {0};

// --------------
// Rx Register
// --------------
#define RxReg_ADDR 0xE2
#define RxReg_Mode_QAM_AUTO 0b1111
#define RxReg_Mode_V22_CALL 0b1011
#define RxReg_Mode_V22_ANS 0b1010
#define RxReg_Mode_BELL 0b0011

union {
    uint16_t Word;

    struct {
        uint16_t BitsParity: 3; //b0
        uint16_t StartStop_Synch: 3;
        uint16_t Equaliser: 3;
        uint16_t RxLevel: 3;
        uint16_t RxMode: 4;
    } __attribute__((packed)) Bits;
} RxReg = {0};

// --------------
// Data Registers
// --------------
#define TxData_ADDR 0xE4   //8bit
#define RxData_ADDR 0xE5   //8bit

// --------------
// Status Register
// --------------
#define StatusReg_ADDR 0xE6
CMX869B_Status StatusReg = {0};

// --------------
// QAM Register
// --------------
#define QamCmdReg_ADDR 0xEA
#define QamMaxBR_14400 0b111
#define QamMaxBR_12000 0b110
#define QamMaxBR_9600 0b101
#define QamMaxBR_2400 0b001
#define QamCall 0b010
#define QamAnswer 0b011
#define QamZeros 0b0000000000

union {
    uint16_t Word;

    struct {
        uint16_t br: 3;
        uint16_t command: 3; //Calling/Answer
        uint16_t zeros: 10;
    } __attribute__((packed)) Bits;
} QamReg = {0};

// --------------
// QAM Status Register
// --------------
#define QamStatusReg_ADDR 0xEB

union {
    uint16_t Word;

    struct {
        uint16_t Mode: 4; //b0
        uint16_t SNR: 3;
        uint16_t Zeros: 3;
        uint16_t Messages: 6;
    } __attribute__((packed)) Bits;
} QamStatusReg = {0};

// ==========================================
// 内部利用関数
// ==========================================
void set_bell(void);

void set_qam_call(void);

void set_qam_answer(void);

static void cbus_write_reg(uint8_t adr, uint16_t data);

static uint16_t cbus_read_reg(uint8_t adr);

static void cbus_write_data(uint8_t data);

static uint8_t cbus_read_data();

static void cbus_tx_int_enable(void);

static void cbus_tx_int_disable(void);

// ==========================================
// グローバル変数
// ==========================================
// 動作モード
static int Is_gse = 0;

//*==========================================
// 公開関数
//==========================================
//初期化
void CMX869B_Init(int is_gse) {
    static int is_loop_back = 0;
    static int is_Cal = 0;
    //モード通知
    Is_gse = is_gse;
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
            QamStatusReg.Word = cbus_read_reg(QamStatusReg_ADDR);
            xprintf("GSE:%d TI:%03d MSG:%02X SNR:%02X MODE:%02X\r\n",
                    Is_gse,
                    tic, QamStatusReg.Bits.Messages,
                    QamStatusReg.Bits.SNR,
                    QamStatusReg.Bits.Mode);
            HAL_Delay(500);
        }
    }
    //タイマ割込起動
    HAL_TIM_Base_Start_IT(&htim);
    xprintf("Initialized\r\n");
}

void CMX869B_putc(char c) {
    cbus_write_data(c);
}

uint8_t CMX869B_getc() {
    uint8_t c = cbus_read_data();
    return c;
}

CMX869B_Status CMX869B_status(void) {
    CMX869B_Status st;
    st.Word = cbus_read_reg(StatusReg_ADDR);
    return st;
};

static uint8_t TxPtr = 0, RxPtr = 0;
static uint8_t tx_buffer[N_TXBuf];
static uint8_t rx_buffer[N_RXBuf];

void CMX869b_write(uint8_t data[], int len) {
    memcpy(tx_buffer, data, len);
    TxPtr = 0;
}

void CMX869b_read(uint8_t data[], int len) {
    memcpy(data, tx_buffer, len);
    RxPtr = 0;
}

//500usecで回す
void CMX869b_poll() {
    HAL_GPIO_WritePin(CPU_MON_GPIO_Port, CPU_MON_Pin, GPIO_PIN_SET);
    StatusReg.Word = cbus_read_reg(StatusReg_ADDR);
    if (StatusReg.Bits.RxDataReady == 1) {
        if (RxPtr < N_RXBuf) {
            rx_buffer[RxPtr++] = CMX869B_getc();
        }
    }
    if (StatusReg.Bits.TxDataReady == 1 && StatusReg.Bits.TxDataUnderflow == 1) {
        if (TxPtr < N_TXBuf) {
            CMX869B_putc(tx_buffer[TxPtr++]);
        }
    }
    HAL_GPIO_WritePin(CPU_MON_GPIO_Port, CPU_MON_Pin, GPIO_PIN_RESET);
}

// ==========================================
// C-BUS操作関数
// ==========================================
static void cbux_transmit_receive(const uint8_t tx_data[], uint8_t rx_data[], int len) {
    SPI_TypeDef *spi = hspi.Instance;
    C_CS_GPIO_Port->BSRR = (uint32_t) C_CS_Pin << 16;
    for (int i = 0; i < len; i++) {
        while (!LL_SPI_IsActiveFlag_TXE(spi)) {
        }
        LL_SPI_TransmitData8(spi, tx_data[i]);
        // 全二重なので受信完了＝送信完了
        while (!LL_SPI_IsActiveFlag_RXNE(spi)) {
        }
        rx_data[i] = LL_SPI_ReceiveData8(spi);
    }
    // CS を上げる前に最後のフレームの完了を待つ
    while (LL_SPI_IsActiveFlag_BSY(spi)) {
    }
    C_CS_GPIO_Port->BSRR = C_CS_Pin;
}

void cbus_write_reg(uint8_t adr, uint16_t data) {
    static uint8_t tx_data[3], rx_data[3];
    tx_data[0] = adr;
    tx_data[1] = (data >> 8) & 0xFF;
    tx_data[2] = (data >> 0) & 0xFF;
    cbux_transmit_receive(tx_data, rx_data, 3);
}

uint16_t cbus_read_reg(uint8_t adr) {
    static uint8_t tx_data[3], rx_data[3];
    tx_data[0] = adr;
    tx_data[1] = tx_data[2] = 0x0;
    cbux_transmit_receive(tx_data, rx_data, 3);
    uint16_t result = (rx_data[1] << 8) + rx_data[2];
    return result;
}

void cbus_write_data(uint8_t data) {
    static uint8_t tx_data[3], rx_data[3];
    tx_data[0] = TxData_ADDR;
    tx_data[1] = tx_data[2] = data;
    cbux_transmit_receive(tx_data, rx_data, 2);
}

uint8_t cbus_read_data() {
    static uint8_t tx_data[2], rx_data[2];
    tx_data[0] = RxData_ADDR;
    tx_data[1] = 0x0;
    cbux_transmit_receive(tx_data, rx_data, 2);
    return rx_data[1];
}

void cbus_tx_int_enable(void) {
    GRE.Bits.IrqMask = 0b001001;
    cbus_write_reg(GRE_ADDR, GRE.Word);
}

void cbus_tx_int_disable(void) {
    GRE.Bits.IrqMask = 0b000001;
    cbus_write_reg(GRE_ADDR, GRE.Word);
}

// ==========================================
// xprintf用
// ==========================================
void uart_putc(unsigned char c) {
    while (!(huart.Instance->ISR & USART_ISR_TXE)) {
    }
    huart.Instance->TDR = c;
}

// ==========================================
// モデムモード
// ==========================================
#define TX_LEVEL 0b111
#define RX_LEVEL 0b111

void set_bell(void) {
    // TX
    TxReg.Bits.TxMode = TxReg_Mode_BELL;
    TxReg.Bits.TxLevel = TX_LEVEL;
    //TxReg.Bits.StartStop = 0b10; //Start-stop, NonParity
    //TxReg.Bits.DataBits = 0b110; //8bit Stop1
    TxReg.Bits.StartStop = 0b11; //Special Mode
    TxReg.Bits.DataBits = 0b110; //HDLC Mode
    cbus_write_reg(TxReg_ADDR, TxReg.Word);
    // RX
    RxReg.Bits.RxMode = RxReg_Mode_BELL;
    RxReg.Bits.RxLevel = RX_LEVEL;
    //RxReg.Bits.StartStop_Synch = 0b110; //Start-stop, NonOverSpeed
    //RxReg.Bits.BitsParity = 0b111; //8bit, NonParity
    RxReg.Bits.StartStop_Synch = 0b111; //Special Mode
    RxReg.Bits.BitsParity = 0b110; //HDLC Mode
    cbus_write_reg(RxReg_ADDR, RxReg.Word);
}

// AutoModem ネゴシエーション必須
void set_qam_answer(void) {
    // TX
    TxReg.Bits.TxMode = TxReg_Mode_QAM_AUTO;
    TxReg.Bits.TxLevel = TX_LEVEL;
    TxReg.Bits.StartStop = 0b10; //Start-stop, NonParity
    TxReg.Bits.DataBits = 0b110; //8bit Stop1
    cbus_write_reg(TxReg_ADDR, TxReg.Word);
    HAL_Delay(10);
    // RX
    RxReg.Bits.RxMode = RxReg_Mode_QAM_AUTO;
    RxReg.Bits.RxLevel = RX_LEVEL;
    RxReg.Bits.StartStop_Synch = 0b110; //Start-stop, NonOverSpeed
    RxReg.Bits.BitsParity = 0b111; //8bit, NonParity
    cbus_write_reg(RxReg_ADDR, RxReg.Word);
    HAL_Delay(10);
    //QAM
    //QamReg.Bits.br = QamMaxBR_14400;
    QamReg.Bits.br = QamMaxBR_2400;
    QamReg.Bits.command = QamAnswer;
    QamReg.Bits.zeros = QamZeros;
    cbus_write_reg(QamCmdReg_ADDR, QamReg.Word);
    HAL_Delay(10);
}

void set_qam_call(void) {
    // TX
    TxReg.Bits.TxMode = TxReg_Mode_QAM_AUTO;
    TxReg.Bits.TxLevel = TX_LEVEL;
    TxReg.Bits.StartStop = 0b10; //Start-stop, NonParity
    TxReg.Bits.DataBits = 0b110; //8bit Stop1
    cbus_write_reg(TxReg_ADDR, TxReg.Word);
    HAL_Delay(10);
    // RX
    RxReg.Bits.RxMode = RxReg_Mode_QAM_AUTO;
    RxReg.Bits.RxLevel = RX_LEVEL;
    RxReg.Bits.StartStop_Synch = 0b110; //Start-stop, NonOverSpeed
    RxReg.Bits.BitsParity = 0b111; //8bit, NonParity
    cbus_write_reg(RxReg_ADDR, RxReg.Word);
    HAL_Delay(10);
    //QAM
    //QamReg.Bits.br = QamMaxBR_14400;
    QamReg.Bits.br = QamMaxBR_2400;
    QamReg.Bits.command = QamCall;
    QamReg.Bits.zeros = QamZeros;
    cbus_write_reg(QamCmdReg_ADDR, QamReg.Word);
    HAL_Delay(10);
}
