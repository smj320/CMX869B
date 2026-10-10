//
// Created by kikuchi on 2026/01/12.
//

#include "stm32f3xx_hal.h"
#include "CMX869B.h"
#include <string.h>
#include "main.h"
#include "xprintf.h"

uint8_t TX_buffer[N_TX_BUFFER];
uint8_t CMD_buffer[N_CMD_BUFFER];
int TX_ptr = N_TX_BUFFER;
int RX_ptr = 0;
int Cmd_Reary = 0;
int Is_gse = 0;

extern UART_HandleTypeDef huart2;
extern TIM_HandleTypeDef htim2;

CMX869B_GRE_TypeDef GRE = {0};
CMX869B_TxReg_TypeDef TxReg = {0};
CMX869B_RxReg_TypeDef RxReg = {0};
CMX869B_QamReg_TypeDef QamReg = {0};
CMX869B_StatusReg_TypeDef StatusReg = {0};
CMX869B_QamStatusReg_TypeDef QamStatusReg = {0};

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

//******************************************************
// C-BUS操作関数
//******************************************************
void CMX869B_Init(int is_gse) {
    // グローバルリセット
    int n_tim2=0;
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
    GRE.Bits.LB = 0;
    //GRE.Bits.LB = 1;
    GRE.Bits.Rst = 0;
    GRE.Bits.IrqEna = 1;
    GRE.Bits.IrqMask = 0b000000;
    CBUS_GRE_WRITE(GRE.Word);
    CBUS_ST_READ(&StatusReg.Word);
    HAL_Delay(1);

    // モデム送受信の設定
    //set_bell();
    //set_v22_loop();
    if (is_gse) {
        set_qam_call();
    }else {
        set_qam_answer();
    }
    //ハンドシェーク監視
    for (int tic=0; tic<2*60; tic++) {
        CBUS_QAM_ST_READ(&QamStatusReg.Word);
        HAL_GPIO_TogglePin(CPU_MON_GPIO_Port, CPU_MON_Pin);
        xprintf("%03d %04X\r\n",
            tic,QamStatusReg.Word);
        HAL_Delay(500);
    }

    //ネゴシエーションの結果取得と送信タイマの設定
    CBUS_QAM_ST_READ(&QamStatusReg.Word);
    n_tim2 = get_qam_itm2(QamStatusReg.Bits.Mode);

    //受信割込許可
    GRE.Bits.IrqMask = 0b000001;
    CBUS_GRE_WRITE(GRE.Word);

    //送信タイマ動作スタート
    __HAL_TIM_SET_AUTORELOAD(&htim2, n_tim2);
    HAL_TIM_Base_Start_IT(&htim2);
}
//******************************************************
// 送信バッファ書き込み
//******************************************************
void CMX869B_write_buffered(const uint8_t data[], int len) {
    for (int l=0; l<len; l++) {
        TX_buffer[l] = data[l];
    }
    TX_ptr = 0;
}

//************************
// 送信ポーリング
//************************
void CMX869B_TIM2_INT(void) {
    if (TX_ptr<N_TX_BUFFER) {
        CBUS_DATA_WRITE(TX_buffer[TX_ptr++]);
    }
}

//************************
// 受信割込
//************************
void CMX869B_RCV_INT(void) {
    static uint16_t rx_data = 0;
    //ステータス確認
    CBUS_ST_READ(&StatusReg.Word);
    //受信データが来ていれば
    if (StatusReg.Bits.RxDataReady == 1) {
        CBUS_DATA_READ(&rx_data);
        CBUS_ST_READ(&StatusReg.Word);
        //GSEなら受信データをすぐUARTに投げる
        if (Is_gse == 1) {
            uart_putc(rx_data);
        } else {
            //Drillならコマンド構成
            if (Cmd_Reary==0) {
                if (RX_ptr == '$') RX_ptr = 0;
                CMD_buffer[RX_ptr] = rx_data;
                if (RX_ptr<(N_CMD_BUFFER-1)) RX_ptr++;
                if (RX_ptr == '#') {
                    CMD_buffer[RX_ptr++] = 0;
                    Cmd_Reary=1;
                    xprintf("%s", CMD_buffer);
                }
            }
        }
    };
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
