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
    uint8_t tx_data = 0, rx_data = 0;
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

        //タスクから一文字
        for (uint8_t i = 10; i < 12; i++) {
            CMX869B_putc(i);
            osDelay(800);
        }
        /*
        //データをセット
        for (int i = 0; i < 10; i++) {
            tx_data =   i;
            //CMX869B_putc(tx_data);
            osDelay(1);
            xStreamBufferSend(xStreamBufferTx, &tx_data, 1, 0);
        }
        //タスクから送ってみる
        for (;;) {
            uint8_t cc = 0x00;
            size_t res = xStreamBufferReceive(
                xStreamBufferTx, &cc, 1, 0);
            if (res == 1) {
                CMX869B_putc(cc);
                osDelay(100);
            } else {
                break;
            }
        }
        //CMX869B_flush_Tx();
        // 待機
        //CMX869B_flush_Tx();
        */
        //osDelay(500);
        // 読んでみる
        for (;;) {
            rx_data = 0x00;
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
