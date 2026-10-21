//
// Created by kikuchi on 2026/10/21.
//
#include "main.h"
#include "TaskCMX86B.h"
#include "cmsis_os.h"
#include "xprintf.h"
#include "task.h"

void TaskHKEntry(void *argument) {
    int num=0;

    // 基準時刻の初期化
    const TickType_t xFrequency = pdMS_TO_TICKS(1000*2); // 1000msをTick数に変換

    // 現在のティック数を取得して初期化
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        //タイマ割込待機
        CMX_putc('A');
        xprintf("\r\nHK:%02d\r\n", num++%10);
        // 1秒待機
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}