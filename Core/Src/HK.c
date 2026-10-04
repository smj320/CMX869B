//
// Created by kikuchi on 2026/09/27.
//
#include "string.h"
#include "main.h"
#include "CMX869b.h"
#include "HK.h"
#include "xprintf.h"
//
extern TIM_HandleTypeDef htim2;

extern uint8_t TX_buffer[];
extern uint8_t RX_buffer[];
extern int TX_ptr;
extern int RX_ptr;

static CMX869B_StatusReg_TypeDef StatusReg;

//
//********************************************
// HK生成とか
//********************************************
void HKLoop() {
    static int count = 0;
    //タイマ動作スタート
    HAL_TIM_Base_Start_IT(&htim2);
    //タイマ周期の変更
    __HAL_TIM_SET_AUTORELOAD(&htim2, 99);

    //１個送信
    //CBUS_WRITE('G');
    for (;;) {
        CBUS_WRITE(count++%10);
        HAL_Delay(20);
    }
}
