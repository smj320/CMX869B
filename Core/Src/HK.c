//
// Created by kikuchi on 2026/09/27.
//
#include "string.h"
#include "main.h"
#include "CMX869b.h"

extern UART_HandleTypeDef huart2;
//
static CMX869B_StatusReg_TypeDef StatusReg = {0};
//
extern int Ptr;
//
//********************************************
// HK生成とか
//********************************************
void StartvHkTask(void *argument) {
    const uint32_t period = 1000; // 1秒（1000ms）周期

    for (;;) {
        //ハートビート確認
        //HAL_GPIO_TogglePin(CPU_MON_GPIO_Port, CPU_MON_Pin);
        //ステータス確認
    }
}
