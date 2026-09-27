//
// Created by kikuchi on 2026/09/27.
//
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"

extern UART_HandleTypeDef huart2;

//********************************************
// モデム監視タスクループ
// 1800Hzの割込から起動される
//********************************************
void HKTaskLoop() {
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        HAL_UART_Transmit(&huart2, (uint8_t*)"HKTaskLoop\r\n", 12, 100);
    }
}