//
// Created by kikuchi on 2026/10/21.
//
#include "main.h"
#include "cmsis_os.h"
#include "xprintf.h"
#include "task.h"
#include "TaskHK.h"
#include "TaskHK_priv.h"
#include "TaskCMX869B.h"

void TaskHKEntry(void *argument) {
    static int num = 0;
    // 基準時刻の初期化
    const TickType_t xFrequency = pdMS_TO_TICKS(1000*2); // 1000msをTick数に変換
    // 現在のティック数を取得して初期化
    TickType_t xLastWakeTime = xTaskGetTickCount();
    for (;;) {
        xprintf("Zap:\r\n");
        // バッファクリアと送信
        xStreamBufferReset(xStreamBufferTx);
        xStreamBufferReset(xStreamBufferRx);
        for (int i = 0; i < 10; i++) {
            uint8_t tx_data = num % 10 + i;
            xStreamBufferSend(xStreamBufferTx, &tx_data, 1, 0);
        }
        CMX869B_putc(1);
        osDelay(100);
        CMX869B_putc(2);
        osDelay(100);
        CMX869B_putc(3);
        // 待機
        osDelay(500);
        // 読んでみる
        for (;;) {
            uint8_t rx_data = 0x00;
            size_t res = xStreamBufferReceive(
                xStreamBufferRx, &rx_data, 1, 0);
            if (res == 1) {
                xprintf("%03d ", rx_data);
            } else {
                xprintf("\r\n");
                break;
            }
        }
        num++;
        // 1秒待機
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}
