//
// Created by kikuchi on 2026/09/27.
//
#include "string.h"
#include "main.h"
#include "cmsis_os.h"
#include "CMX869b.h"
#include "xprintf.h"
#include "task.h"

extern UART_HandleTypeDef huart2;
extern osMessageQueueId_t txQueueHandle;
extern osMessageQueueId_t rxQueueHandle;
//
static CMX869B_GRE_TypeDef GRE = {0};
static CMX869B_StatusReg_TypeDef StatusReg = {0};

//********************************************
// HK生成とか
//********************************************
void StartvHkTask(void *argument) {
    const uint32_t period = 1000; // 1秒（1000ms）周期
    uint32_t tick = osKernelGetTickCount();
    int txd0, txf0, irq0;

    for (;;) {
        //ハートビート確認
        HAL_GPIO_TogglePin(CPU_MON_GPIO_Port, CPU_MON_Pin);
        //ステータス確認
        receive_status(&StatusReg);
        irq0 = StatusReg.Bits.IRQ;
        txd0 = StatusReg.Bits.TxDataReady;
        txf0 = StatusReg.Bits.TxDataOverflow;
        send_data('A');
        //receive_status(&StatusReg);
        //txd1 = StatusReg.Bits.TxDataReady;
        //txf1 = StatusReg.Bits.TxDataOverflow;
        //報告
        vTaskSuspendAll();
        xprintf("HK  %d %d %d\r\n", txd0, txf0);
        xTaskResumeAll();
        //周期待機
        tick += period;
        osDelayUntil(tick);
    }
}
