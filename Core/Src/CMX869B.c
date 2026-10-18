//
// Created by kikuchi on 2026/01/12.
//

#include "stm32f3xx_hal.h"
#include "CMX869B.h"
#include <string.h>
#include "main.h"
#include "xprintf.h"

// ==========================================
// レジスタとビットフィールド
// ==========================================
// General Reset
#define General_Reset_ADDR 0x01

// Global Register
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

// Tx Register
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

// Rx Register
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

#define TxData_ADDR 0xE4   //8bit
#define RxData_ADDR 0xE5   //8bit

// Status Register
#define StatusReg_ADDR 0xE6

union {
    uint16_t Word;

    struct {
        uint16_t FSKDemodulatorOutput: 1; // b0
        uint16_t Char2Mode: 1; // b1
        uint16_t Char2EvenHasParity: 1; // b2
        uint16_t Char2EvenParity: 1; // b3
        uint16_t FrameError: 1; // b4
        uint16_t RxDataOverflow: 1; // b5　 MASK b00
        uint16_t RxDataReady: 1; // b6 　MASK b00
        uint16_t b7_Unused: 1; // b7
        uint16_t b8_RightJustify: 1; // b8
        uint16_t b9_FrameSyncDetect: 1; // b9
        uint16_t EnergyDetect: 1; // b10
        uint16_t TxDataUnderflow: 1; // b11　MASK b03
        uint16_t TxDataReady: 1; // b12　MASK b03
        uint16_t ProgrammingFlag: 1; // b13
        uint16_t RingDetect: 1; // b14
        uint16_t IRQ: 1; // b15
    } __attribute__((packed)) Bits;
} StatusReg = {0};

// QAM Register
#define QamCmdReg_ADDR 0xEA
#define QamMaxBR_14400 0b111
#define QamMaxBR_12000 0b110
#define QamMaxBR_9600 0b101
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

// QAM Status Register
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

// ==========================================
// グローバル変数
// ==========================================
// 動作モード
static int Is_gse = 0;

// 送受信バッファ
uint8_t TX_buffer[CMX_TX_BUFFER];
uint8_t RX_buffer[CMX_TX_BUFFER];
static int TxPtr = CMX_TX_BUFFER;
static int RxPtr = 0;

//*==========================================
// 公開関数
//==========================================
//初期化
void CMX869B_Init(int is_gse) {
    //モード通知
    Is_gse = is_gse;

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
    //GRE.Bits.LB = 0;
    GRE.Bits.LB = 1;
    GRE.Bits.Rst = 0;
    GRE.Bits.IrqEna = 1;
    GRE.Bits.IrqMask = 0b000000;
    cbus_write_reg(GRE_ADDR, GRE.Word);
    StatusReg.Word = cbus_read_reg(StatusReg_ADDR);
    HAL_Delay(1);

    // モデム送受信の設定
#define MD_CAL 1
#define MD_LOOP 2
    //#define MODE MD_CAL
#define MODE MD_LOOP
#if MODE == MD_CAL
    set_bell();
    while (1);
#elif MODE == MD_LOOP
    set_bell();
    //set_v22_loop();
#else
    if (is_gse) {
        set_qam_call();
    } else {
        set_qam_answer();
    }
    //ハンドシェーク監視
    for (uint8_t tic = 0; tic < 240; tic++) {
        CBUS_QAM_ST_READ(&QamStatusReg.Word);
        HAL_GPIO_TogglePin(CPU_MON_GPIO_Port, CPU_MON_Pin);
        xprintf("GSE:%d TI:%03d MSG:%02X SNR:%02X MODE:%02X\r\n",
                Is_gse,
                tic, QamStatusReg.Bits.Messages,
                QamStatusReg.Bits.SNR,
                QamStatusReg.Bits.Mode);
        HAL_Delay(500);
        tic++;
    }

    //ネゴシエーションの結果取得と送信タイマの設定
    CBUS_QAM_ST_READ(&QamStatusReg.Word);
    n_tim2 = get_qam_itm2(QamStatusReg.Bits.Mode);
#endif
    //受信割込許可
    //GRE.Bits.IrqMask = 0b000001;
    GRE.Bits.IrqMask = 0b001001;
    cbus_write_reg(GRE_ADDR, GRE.Word);
    StatusReg.Word = cbus_read_reg(StatusReg_ADDR);

    for (int i = 0; i < CMX_TX_BUFFER; i++) TX_buffer[i] = i;
    xprintf("Initialized\r\n");
}

// バッファライト
void CMX869B_write_data(uint8_t *tx_data, int len) {
    memcpy(TX_buffer, tx_data, len);
    TxPtr = 1;
    //一文字書く
}

// バッファリード
void CMX869B_read_data(uint8_t *rx_data, int len) {
    memcpy(rx_data, RX_buffer, len);
}

void CMX869B_read_reset(void) {
    RxPtr = 0;
}

// ==========================================
// 受信割込
// ==========================================
void CMX869B_RCV_INT(void) {
    static uint16_t rx_data = 0;
    HAL_GPIO_WritePin(CPU_MON_GPIO_Port, CPU_MON_Pin, GPIO_PIN_SET);
    //送信系データがないか確認
    StatusReg.Word = cbus_read_reg(StatusReg_ADDR);
    if (StatusReg.Bits.RxDataReady == 1 || StatusReg.Bits.RxDataOverflow == 1) {
        rx_data = cbus_read_data();
        if (StatusReg.Bits.RxDataReady == 1) {
            xprintf("RX:%03d\r\n", rx_data);
        }
        //念のため割込フラグを戻す
        StatusReg.Word = cbus_read_reg(StatusReg_ADDR);
        HAL_GPIO_WritePin(CPU_MON_GPIO_Port, CPU_MON_Pin, GPIO_PIN_RESET);
        return;
    }

    //割込処理中にデータ届いている農政もあるので、もう一回見に行く
    StatusReg.Word = cbus_read_reg(StatusReg_ADDR);
    if (StatusReg.Bits.TxDataReady == 1 || StatusReg.Bits.TxDataUnderflow == 1) {
        if (StatusReg.Bits.TxDataReady == 1) {
            if (TxPtr < CMX_TX_BUFFER) {
                xprintf("TX:%03d\r\n", TX_buffer[TxPtr]);
                cbus_write_data(TX_buffer[TxPtr++]);
            }
        }
        //念のため割込フラグを戻す
        StatusReg.Word = cbus_read_reg(StatusReg_ADDR);
        HAL_GPIO_WritePin(CPU_MON_GPIO_Port, CPU_MON_Pin, GPIO_PIN_RESET);
    }
}

// ==========================================
// C-BUS操作関数
// ==========================================
static void cbux_transmit_receive(const uint8_t tx_data[], uint8_t rx_data[], int len) {
    C_CS_GPIO_Port->BSRR = (uint32_t) C_CS_Pin << 16;
    for (int i = 0; i < len; i++) {
        *(volatile uint8_t *) &(hspi.Instance->DR) = tx_data[i];
        while (!(hspi.Instance->SR & SPI_SR_TXE)) {
        };
        while (hspi.Instance->SR & SPI_SR_BSY) {
        };
        rx_data[i] = *(volatile uint8_t *) &(hspi.Instance->DR);
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
    cbux_transmit_receive(tx_data, rx_data, 2);
    uint16_t result =  (rx_data[1] << 8)  + rx_data[2];
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
    TxReg.Bits.StartStop = 0b10; //Start-stop, NonParity
    TxReg.Bits.DataBits = 0b110; //8bit Stop1
    cbus_write_reg(TxReg_ADDR, TxReg.Word);
    // RX
    RxReg.Bits.RxMode = RxReg_Mode_BELL;
    RxReg.Bits.RxLevel = RX_LEVEL;
    RxReg.Bits.StartStop_Synch = 0b110; //Start-stop, NonOverSpeed
    RxReg.Bits.BitsParity = 0b111; //8bit, NonParity
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
    // RX
    RxReg.Bits.RxMode = RxReg_Mode_QAM_AUTO;
    RxReg.Bits.RxLevel = RX_LEVEL;
    RxReg.Bits.StartStop_Synch = 0b110; //Start-stop, NonOverSpeed
    RxReg.Bits.BitsParity = 0b111; //8bit, NonParity
    cbus_write_reg(RxReg_ADDR, RxReg.Word);
    //QAM
    QamReg.Bits.br = QamMaxBR_14400;
    QamReg.Bits.command = QamAnswer;
    QamReg.Bits.zeros = QamZeros;
    cbus_write_reg(QamCmdReg_ADDR, QamReg.Word);
}

void set_qam_call(void) {
    // TX
    TxReg.Bits.TxMode = TxReg_Mode_QAM_AUTO;
    TxReg.Bits.TxLevel = TX_LEVEL;
    TxReg.Bits.StartStop = 0b10; //Start-stop, NonParity
    TxReg.Bits.DataBits = 0b110; //8bit Stop1
    cbus_write_reg(TxReg_ADDR, TxReg.Word);
    // RX
    RxReg.Bits.RxMode = RxReg_Mode_QAM_AUTO;
    RxReg.Bits.RxLevel = RX_LEVEL;
    RxReg.Bits.StartStop_Synch = 0b110; //Start-stop, NonOverSpeed
    RxReg.Bits.BitsParity = 0b111; //8bit, NonParity
    cbus_write_reg(RxReg_ADDR, RxReg.Word);
    //QAM
    QamReg.Bits.br = QamMaxBR_14400;
    QamReg.Bits.command = QamCall;
    QamReg.Bits.zeros = QamZeros;
    cbus_write_reg(QamCmdReg_ADDR, QamReg.Word);
}
