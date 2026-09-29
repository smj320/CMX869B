//
// Created by kikuchi on 2026/09/27.
//
#include "string.h"
#include "main.h"
#include "cmsis_os.h"
#include "CMX869b.h"
#include "task.h"

extern UART_HandleTypeDef huart2;
extern osMessageQueueId_t txQueueHandle;
extern osMessageQueueId_t rxQueueHandle;
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
    uint32_t tick = osKernelGetTickCount();

    for (;;) {
        //ハートビート確認
        //HAL_GPIO_TogglePin(CPU_MON_GPIO_Port, CPU_MON_Pin);
        //ステータス確認
        tick += period;
        osDelayUntil(tick);
    }
}
