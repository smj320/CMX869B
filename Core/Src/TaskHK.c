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
    uint8_t tx_buf[TX_BUF_SIZE] = {0};
    uint8_t rx_buf[RX_BUF_SIZE] = {0};

    // 基準時刻の初期化と現在のティック数を取得
    const TickType_t xFrequency = pdMS_TO_TICKS(1000*2); // 1000msをTick数に変換
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        xprintf("Zap:\r\n");
        for (int i = 0; i < TX_BUF_SIZE; i++) {
            tx_buf[i] = Is_gse_mode ? 0x80 + i : i;  // 発呼側と応答側で値を変えて区別する
        }
        xQueueSend(xQueueTx, tx_buf, 0);
        CMX869B_flush_Tx();

        // 読んでみる
        osDelay(500);
        if (xQueueReceive(xQueueRx, rx_buf, 0)==pdPASS) {
            for (int i = 0; i < RX_BUF_SIZE; i++) {
                xprintf("%02X ", rx_buf[i]);
            }
            xprintf("\r\n");
        }

        // 回線状態の表示(受信バイト数とQAMステータス)
        uint8_t mode, snr, msg;
        CMX869B_get_qam_status(&mode, &snr, &msg);
        xprintf("SNR:%d MODE:%02X MSG:%02X\r\n", snr, mode, msg);

        // 1秒待機
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}
