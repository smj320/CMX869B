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
    uint16_t  Word;
    struct {
        uint16_t IrqMask : 6; //b0
        uint16_t IrqEna : 1;
        uint16_t Rst : 1;
        uint16_t Pwr : 1;
        uint16_t RlyDrv : 1;
        uint16_t _2C : 1;
        uint16_t LB : 1;
        uint16_t  Zero : 1;
        uint16_t  PatDet : 1;
        uint16_t  HighGain : 1;
        uint16_t  Equ : 1;
    } __attribute__((packed)) Bits;
} GRE = {0};

// Tx Register
#define TxReg_ADDR 0xE1
#define TxReg_Mode_QAM_AUTO 0b1111
#define TxReg_Mode_V22_ANS 0b1011
#define TxReg_Mode_V22_CALL 0b1010
#define TxReg_Mode_BELL 0b0011
union {
    uint16_t  Word;
    struct {
        uint16_t DataBits : 3; //b0
        uint16_t StartStop : 2;
        uint16_t Scramble : 2;
        uint16_t GardTone : 2;
        uint16_t TxLevel : 3;
        uint16_t TxMode : 4;
    } __attribute__((packed)) Bits;
} TxReg = {0};

// Rx Register
#define RxReg_ADDR 0xE2
#define RxReg_Mode_QAM_AUTO 0b1111
#define RxReg_Mode_V22_CALL 0b1011
#define RxReg_Mode_V22_ANS 0b1010
#define RxReg_Mode_BELL 0b0011
union {
    uint16_t  Word;
    struct {
        uint16_t BitsParity : 3; //b0
        uint16_t StartStop_Synch : 3;
        uint16_t Equaliser : 3;
        uint16_t RxLevel : 3;
        uint16_t RxMode : 4;
    } __attribute__((packed)) Bits;
} RxReg = {0};

#define TxData_ADDR 0xE4   //8bit
#define RxData_ADDR 0xE5   //8bit

// Status Register
#define StatusReg_ADDR 0xE6
union {
    uint16_t  Word;
    struct {
        uint16_t FSKDemodulatorOutput : 1; // b0
        uint16_t Char2Mode : 1;            // b1
        uint16_t Char2EvenHasParity : 1;   // b2
        uint16_t Char2EvenParity : 1;      // b3
        uint16_t FrameError : 1;           // b4
        uint16_t RxDataOverflow : 1;       // b5　 MASK b00
        uint16_t RxDataReady : 1;          // b6 　MASK b00
        uint16_t b7_Unused : 1;            // b7
        // MSB (b8-b15)
        uint16_t b8_RightJustify : 1;      // b8
        uint16_t b9_FrameSyncDetect : 1;   // b9
        uint16_t EnergyDetect : 1;         // b10
        uint16_t TxDataUnderflow : 1;       // b11　MASK b03
        uint16_t TxDataReady : 1;          // b12　MASK b03
        uint16_t ProgrammingFlag : 1;      // b13
        uint16_t RingDetect : 1;           // b14
        uint16_t IRQ : 1;                  // b15

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
    uint16_t  Word;
    struct {
        uint16_t br : 3;
        uint16_t command : 3; //Calling/Answer
        uint16_t zeros : 10;
    }  __attribute__((packed)) Bits;
} QamReg = {0};

// QAM Status Register
#define QamStatusReg_ADDR 0xEB
union {
    uint16_t  Word;
    struct {
        uint16_t Mode : 4; //b0
        uint16_t SNR : 3;
        uint16_t Zeros : 3;
        uint16_t Messages : 6;
    } __attribute__((packed)) Bits;
} QamStatusReg = {0};

// ==========================================
// 内部利用関数
// ==========================================
void set_bell(void);
void set_v22_call(void);
void set_v22_answer(void);
void set_v22_loop(void);
void set_qam_call(void);
void set_qam_answer(void);
int  get_qam_itm2(uint16_t mode);

void cbus_write(uint8_t addr, uint16_t data);
void cbus_read(uint8_t addr, uint16_t *data);
#define CBUS_GRE_WRITE(x) cbus_write(GRE_ADDR, x);
#define CBUS_DATA_WRITE(x) cbus_write(TxData_ADDR, x);
#define CBUS_DATA_READ(x) cbus_read(RxData_ADDR, x);
#define CBUS_ST_READ(x) cbus_read(StatusReg_ADDR, x);
#define CBUS_QAM_ST_READ(x) cbus_read(QamStatusReg_ADDR, x);

// ==========================================
// グローバル変数
// ==========================================
uint8_t TX_buffer[CMX_TX_BUFFER];
uint8_t RX_buffer[CMX_TX_BUFFER];
static int TxPtr = CMX_TX_BUFFER;
static int Is_gse = 0;

//*==========================================
// 公開関数
//==========================================
//初期化
void CMX869B_Init(int is_gse) {
    // グローバルリセット
    cbus_write(General_Reset_ADDR, 0);

    //モード通知
    Is_gse = is_gse;

    // ステータス確認
    CBUS_ST_READ(&StatusReg.Word);

    // Pwr=1,Rst=1で20ms以上保持して内部ロジックを確定させる(データシート GCR b8)
    // ここで水晶が発振する。
    GRE.Bits.Pwr = 1;
    GRE.Bits.Rst = 1;
    CBUS_GRE_WRITE(GRE.Word);
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
    CBUS_GRE_WRITE(GRE.Word);
    CBUS_ST_READ(&StatusReg.Word);
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
    CBUS_GRE_WRITE(GRE.Word);
    CBUS_ST_READ(&StatusReg.Word);

    //送信タイマ動作スタート
    //__HAL_TIM_SET_AUTORELOAD(&htim2, n_tim2);
    //HAL_TIM_Base_Start_IT(&htim2);
    for (int i = 0; i < CMX_TX_BUFFER; i++) TX_buffer[i] = i;
    xprintf("Initialized\r\n");
}

// バッファライト
void CMX869B_write(uint8_t *tx_data, int len) {
    memcpy(TX_buffer, tx_data, len);
    TxPtr = 1;
}
// バッファリード
void CMX869B_read(uint8_t *rx_data, int len) {
    memcpy(rx_data, RX_buffer, len);
}
//************************
// 受信割込
//************************
void CMX869B_RCV_INT(void) {
    static uint16_t rx_data = 0;
    HAL_GPIO_WritePin(CPU_MON_GPIO_Port, CPU_MON_Pin, GPIO_PIN_SET);
    //送信系データがないか確認
    CBUS_ST_READ(&StatusReg.Word);
    if (StatusReg.Bits.RxDataReady == 1 || StatusReg.Bits.RxDataOverflow == 1) {
        CBUS_DATA_READ(&rx_data);
        if (StatusReg.Bits.RxDataReady == 1) {
            xprintf("RX:%03d\r\n", rx_data);
        }
        //念のため割込フラグを戻す
        CBUS_ST_READ(&StatusReg.Word);
        HAL_GPIO_WritePin(CPU_MON_GPIO_Port, CPU_MON_Pin, GPIO_PIN_RESET);
        return;
    }

    //割込処理中にデータ届いている農政もあるので、もう一回見に行く
    CBUS_ST_READ(&StatusReg.Word);
    if (StatusReg.Bits.TxDataReady == 1 || StatusReg.Bits.TxDataUnderflow == 1) {
        if (StatusReg.Bits.TxDataReady == 1) {
            if (TxPtr < CMX_TX_BUFFER) {
                xprintf("TX:%03d\r\n", TX_buffer[TxPtr]);
                CBUS_DATA_WRITE(TX_buffer[TxPtr++]);
            }
        }
        //念のため割込フラグを戻す
        CBUS_ST_READ(&StatusReg.Word);
        HAL_GPIO_WritePin(CPU_MON_GPIO_Port, CPU_MON_Pin, GPIO_PIN_RESET);
    }
}

//******************************************************
// C-BUS操作関数
//******************************************************
static void cbus_raw_write(uint16_t c, int n_bits) {
    uint8_t bit;
    for (int i = n_bits - 1; 0 <= i; i--) {
        bit = (c >> (i)) & 0x01;
        HAL_GPIO_WritePin(C_MOSI_GPIO_Port, C_MOSI_Pin, bit);
        HAL_GPIO_WritePin(C_CLK_GPIO_Port, C_CLK_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(C_CLK_GPIO_Port, C_CLK_Pin, GPIO_PIN_RESET);
    }
    HAL_GPIO_WritePin(C_MOSI_GPIO_Port, C_MOSI_Pin, GPIO_PIN_RESET);
}

static void cbus_raw_read(uint16_t *c, int n_bits) {
    int i;
    uint16_t bit, cc;
    for (i = n_bits - 1, cc = 0; 0 <= i; i--) {
        HAL_GPIO_WritePin(C_CLK_GPIO_Port, C_CLK_Pin, GPIO_PIN_SET);
        bit = HAL_GPIO_ReadPin(C_MISO_GPIO_Port, C_MISO_Pin);
        //buf[i] = bit;
        cc = cc + (bit << i);
        HAL_GPIO_WritePin(C_CLK_GPIO_Port, C_CLK_Pin, GPIO_PIN_RESET);
    }
    *c = cc;
}

// C-BUSの操作
void cbus_write(uint8_t adr, uint16_t data) {
    int n_bits = adr == TxData_ADDR ? 8 : 16;
    HAL_GPIO_WritePin(C_CS_GPIO_Port, C_CS_Pin, GPIO_PIN_RESET);
    cbus_raw_write(adr, 8);
    if (adr != General_Reset_ADDR) {
        cbus_raw_write(data, n_bits);
    }
    HAL_GPIO_WritePin(C_CS_GPIO_Port, C_CS_Pin, GPIO_PIN_SET);
}

void cbus_read(uint8_t adr, uint16_t *data) {
    int n_bits = adr == RxData_ADDR ? 8 : 16;
    HAL_GPIO_WritePin(C_CS_GPIO_Port, C_CS_Pin, GPIO_PIN_RESET);
    cbus_raw_write(adr, 8);
    cbus_raw_read(data, n_bits);
    HAL_GPIO_WritePin(C_CS_GPIO_Port, C_CS_Pin, GPIO_PIN_SET);
}

//************************
// xprintf用
//************************
void uart_putc(unsigned char c) {
    while (!(huart.Instance->ISR & USART_ISR_TXE)) {
    }
    huart.Instance->TDR = c;
}

//************************
// モデムモード
//************************
#define TX_LEVEL 0b111
#define RX_LEVEL 0b111

void set_bell(void) {
    // TX
    TxReg.Bits.TxMode = TxReg_Mode_BELL;
    TxReg.Bits.TxLevel = TX_LEVEL;
    TxReg.Bits.StartStop = 0b10; //Start-stop, NonParity
    TxReg.Bits.DataBits = 0b110; //8bit Stop1
    cbus_write(TxReg_ADDR, TxReg.Word);
    // RX
    RxReg.Bits.RxMode = RxReg_Mode_BELL;
    RxReg.Bits.RxLevel = RX_LEVEL;
    RxReg.Bits.StartStop_Synch = 0b110; //Start-stop, NonOverSpeed
    RxReg.Bits.BitsParity = 0b111; //8bit, NonParity
    cbus_write(RxReg_ADDR, RxReg.Word);
}

// 2400 bps 全二重
// ネゴシエーション不用なので、ブチ切りでもいける。
void set_v22_call(void) {
    // TX
    TxReg.Bits.TxMode = TxReg_Mode_V22_CALL;
    TxReg.Bits.TxLevel = TX_LEVEL;
    TxReg.Bits.StartStop = 0b10; //Start-stop, NonParity
    TxReg.Bits.DataBits = 0b110; //8bit Stop1
    cbus_write(TxReg_ADDR, TxReg.Word);
    // RX
    RxReg.Bits.RxMode = RxReg_Mode_V22_CALL;
    RxReg.Bits.RxLevel = RX_LEVEL;
    RxReg.Bits.StartStop_Synch = 0b110; //Start-stop, NonOverSpeed
    RxReg.Bits.BitsParity = 0b111; //8bit, NonParity
    cbus_write(RxReg_ADDR, RxReg.Word);
}

void set_v22_ans(void) {
    // TX
    TxReg.Bits.TxMode = TxReg_Mode_V22_ANS;
    TxReg.Bits.TxLevel = TX_LEVEL;
    TxReg.Bits.StartStop = 0b10; //Start-stop, NonParity
    TxReg.Bits.DataBits = 0b110; //8bit Stop1
    cbus_write(TxReg_ADDR, TxReg.Word);
    // RX
    RxReg.Bits.RxMode = RxReg_Mode_V22_ANS;
    RxReg.Bits.RxLevel = RX_LEVEL;
    RxReg.Bits.StartStop_Synch = 0b110; //Start-stop, NonOverSpeed
    RxReg.Bits.BitsParity = 0b111; //8bit, NonParity
    cbus_write(RxReg_ADDR, RxReg.Word);
}

void set_v22_loop(void) {
    // TX
    TxReg.Bits.TxMode = TxReg_Mode_V22_ANS;
    TxReg.Bits.TxLevel = TX_LEVEL;
    TxReg.Bits.StartStop = 0b10; //Start-stop, NonParity
    TxReg.Bits.DataBits = 0b110; //8bit Stop1
    cbus_write(TxReg_ADDR, TxReg.Word);
    // RX
    RxReg.Bits.RxMode = RxReg_Mode_V22_CALL;
    RxReg.Bits.RxLevel = RX_LEVEL;
    RxReg.Bits.StartStop_Synch = 0b110; //Start-stop, NonOverSpeed
    RxReg.Bits.BitsParity = 0b111; //8bit, NonParity
    cbus_write(RxReg_ADDR, RxReg.Word);
}

// Auto modem
void set_qam_answer(void) {
    // TX
    TxReg.Bits.TxMode = TxReg_Mode_QAM_AUTO;
    TxReg.Bits.TxLevel = TX_LEVEL;
    TxReg.Bits.StartStop = 0b10; //Start-stop, NonParity
    TxReg.Bits.DataBits = 0b110; //8bit Stop1
    cbus_write(TxReg_ADDR, TxReg.Word);
    // RX
    RxReg.Bits.RxMode = RxReg_Mode_QAM_AUTO;
    RxReg.Bits.RxLevel = RX_LEVEL;
    RxReg.Bits.StartStop_Synch = 0b110; //Start-stop, NonOverSpeed
    RxReg.Bits.BitsParity = 0b111; //8bit, NonParity
    cbus_write(RxReg_ADDR, RxReg.Word);
    //QAM
    QamReg.Bits.br = QamMaxBR_14400;
    QamReg.Bits.command = QamAnswer;
    QamReg.Bits.zeros = QamZeros;
    cbus_write(QamCmdReg_ADDR, QamReg.Word);
}

void set_qam_call(void) {
    // TX
    TxReg.Bits.TxMode = TxReg_Mode_QAM_AUTO;
    TxReg.Bits.TxLevel = TX_LEVEL;
    TxReg.Bits.StartStop = 0b10; //Start-stop, NonParity
    TxReg.Bits.DataBits = 0b110; //8bit Stop1
    cbus_write(TxReg_ADDR, TxReg.Word);
    // RX
    RxReg.Bits.RxMode = RxReg_Mode_QAM_AUTO;
    RxReg.Bits.RxLevel = RX_LEVEL;
    RxReg.Bits.StartStop_Synch = 0b110; //Start-stop, NonOverSpeed
    RxReg.Bits.BitsParity = 0b111; //8bit, NonParity
    cbus_write(RxReg_ADDR, RxReg.Word);
    //QAM
    QamReg.Bits.br = QamMaxBR_14400;
    QamReg.Bits.command = QamCall;
    QamReg.Bits.zeros = QamZeros;
    cbus_write(QamCmdReg_ADDR, QamReg.Word);
}

int get_qam_itm2(uint16_t mode) {
    int bps = 0;
    int tim2 = 0;
    switch (mode) {
        case 0b1111: //14400
            bps = 14400;
            break;
        case 0b1110: //12000
            bps = 12000;
            break;
        case 0b1101: //9600
        case 0b1100:
            bps = (int)(9600*0.8);
            break;
        case 0b1011: //7200
            bps = 7200;
            break;
        case 0b1010: //4800
            bps = 4800;
            break;
        case 0b1001: //2400
            bps = 2400;
            break;
        default:
            bps = 1200;
            break;
    }
    tim2 = 100*1200/bps-1;
    return tim2;
}
