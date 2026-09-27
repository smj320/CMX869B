//
// Created by kikuchi on 2026/09/27.
//
#include "string.h"
#include "main.h"
#include "cmsis_os.h"
#include "task.h"
#include "queue.h"

extern UART_HandleTypeDef huart2;
extern osMessageQueueId_t txQueueHandle;
extern osMessageQueueId_t rxQueueHandle;


//********************************************
// モデム監視タスクループ
// 1800Hzの割込から起動される
//********************************************
void HKTaskLoop() {
    char *message = "ABCDEFG";
    for (;;) {
        //INTからのウェイクアップを待機
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        //TXキューにデータを書き込む
        for (int i = 0; i < strlen(message); i++) {
            xQueueSend(txQueueHandle,&message[i],0);
        }
    }
}