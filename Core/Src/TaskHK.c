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
    int num = 0;
    size_t res;
    // 基準時刻の初期化
    const TickType_t xFrequency = pdMS_TO_TICKS(1000*2); // 1000msをTick数に変換
    // 現在のティック数を取得して初期化
    TickType_t xLastWakeTime = xTaskGetTickCount();
    for (;;) {
        //タイマ割込待機
        xprintf("Zap:\r\n");
        uint8_t c = num++ % 100;
        //バッファクリアと送信
        //RxOverflowをReadでクリアしているので、割込は毎回発生する。
        //そのタイミングでTxDataReadyでデータがが到達していれば送信される。
        xStreamBufferReset(xStreamBufferTx);
        xStreamBufferSend(xStreamBufferRx, &c, 1, 0);
        CMX869B_flush_Tx(0x55);
        //待機
        osDelay(500);
        //読んでみる
        do {
            res = xStreamBufferReceive(xStreamBufferRx, &c, 1, 0);
            if (res != 0)xprintf("%02X", c);
        } while (res != 0);
        xprintf("\r\n", c);

        // 1秒待機
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}
