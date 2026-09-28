//
// Created by kikuchi on 2026/09/27.
//
#include "string.h"
#include "main.h"
#include "cmsis_os.h"
#include "CMX869b.h"
#include "task.h"
#include "queue.h"

extern UART_HandleTypeDef huart2;
extern osMessageQueueId_t txQueueHandle;
extern osMessageQueueId_t rxQueueHandle;
//
static CMX869B_StatusReg_TypeDef StatusReg = {0};

//********************************************
// モデム監視タスクループ
// 1800Hzの割込から起動される
//********************************************
void HKTaskLoop() {
    uint8_t message;
    for (;;) {
        //INTからのウェイクアップを待機
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        //ハートビート確認
        HAL_GPIO_TogglePin(CPU_MON_GPIO_Port, CPU_MON_Pin);

        //TXキューにデータを書き込む
        for (int i = 0; i < 80; i++) {
            //xQueueSend(txQueueHandle,&message[i],0);
            message = '0' + i%10;
            send_data(message);
            //20だとうまくいく。15だと途中でとまり、10だとゴミが入る。
            osDelay(11);
        }
        //tx_int_enable();
    }
}