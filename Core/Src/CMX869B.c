//
// Created by kikuchi on 2026/01/12.
//

#include "stm32f3xx_hal.h"
#include "CMX869B.h"
#include <string.h>
#include "main.h"
#include "xprintf.h"

uint8_t Is_1stInt=1;
uint8_t TX_buffer[N_TX_BUFFER];
uint8_t RX_buffer[N_RX_BUFFER];
int TX_ptr = 0;
int RX_ptr = 0;

extern UART_HandleTypeDef huart2;
extern TIM_HandleTypeDef htim2;

CMX869B_GRE_TypeDef GRE = {0};
CMX869B_TxReg_TypeDef TxReg = {0};
CMX869B_RxReg_TypeDef RxReg = {0};
CMX869B_QamReg_TypeDef QamReg = {0};
CMX869B_StatusReg_TypeDef StatusReg = {0};
CMX869B_QamStatusReg_TypeDef QamStatusReg = {0};
//
extern int MODEM_MODE_GSE;

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

void CMX869B_Init(void) {
    // グローバルリセット
    uint16_t rx_data;
    cbus_write(General_Reset_ADDR, 0);

    // ステータス確認
    CBUS_ST_READ(&StatusReg.Word);

    // リセットビットでリセット
    GRE.Bits.Rst = 1;
    CBUS_GRE_WRITE(GRE.Word);
    HAL_Delay(1);

    //send_cmdで、うまくいくと22pinが発振する
    GRE.Bits.Pwr = 1;
    GRE.Bits.HighGain = 1;
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
    //set_bell();
    //set_v22_ans();
    //set_v22_call();
    //set_v22_loop();
    set_qam_answer();
    //set_qam_call();

    //ネゴシエーションの結果取得と送信タイマの設定
    //失敗した場合はv22にフォールバック
    HAL_Delay(10000);
    CBUS_QAM_ST_READ(&QamStatusReg.Word);
    int bps = get_qam_bps(QamStatusReg.Bits.Mode);
    if (bps == 0) {
        bps = 1200;
        set_v22_loop();
        __HAL_TIM_SET_AUTORELOAD(&htim2, bps);
    }else {
        __HAL_TIM_SET_AUTORELOAD(&htim2, bps);
    }

    //受信割込許可
    GRE.Bits.IrqMask = 0b000001;
    CBUS_GRE_WRITE(GRE.Word);

    //送信タイマ動作スタート
    //HAL_TIM_Base_Start_IT(&htim2);
}

void EXEC_C_INT(void) {
    static uint16_t rx_data = 0;
    //ステータス確認
    CBUS_ST_READ(&StatusReg.Word);
    //受信データが来ていれば
    if (StatusReg.Bits.RxDataReady == 1) {
        CBUS_DATA_READ(&rx_data);
        CBUS_ST_READ(&StatusReg.Word);
        xprintf("%d", rx_data);
    };
}
