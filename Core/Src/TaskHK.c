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

    //ジャンパークローズ(0)の場合はGSE
    int is_gse = HAL_GPIO_ReadPin(MODEM_MODE_GPIO_Port, MODEM_MODE_Pin) == 0 ? 1 : 0;

    TickType_t xLastWakeTime = xTaskGetTickCount();
    for (;;) {
        xprintf("Zap:\r\n");
        // バッファクリアと送信
        xStreamBufferReset(xStreamBufferTx);
        //ストリームバッファにデータを詰めて送信開始
        //送信間隔はTxDataReady割込でTaskCMX869Bが制御する
        for (int i = 0; i < 10; i++) {
            tx_data = is_gse ? 0x80 + i : i;  // 発呼側と応答側で値を変えて区別する
            xStreamBufferSend(xStreamBufferTx, &tx_data, 1, 0);
        }
        CMX869B_flush_Tx();
        osDelay(500);
        // 読んでみる
        int rx_count = 0;
        for (;;) {
            rx_data = 0x00;
            size_t res = xStreamBufferReceive(
                xStreamBufferRx, &rx_data, 1, pdMS_TO_TICKS(50));  // 1バイト分より長く待つ
            if (res == 1) {
                xprintf("%02X ", rx_data);
                rx_count++;
            } else {
                xprintf("\r\n");
                break;
            }
        }
        // 回線状態の表示(受信バイト数とQAMステータス)
        uint8_t mode, snr, msg;
        CMX869B_get_qam_status(&mode, &snr, &msg);
        xprintf("RX:%d SNR:%d MODE:%02X MSG:%02X\r\n", rx_count, snr, mode, msg);
        xStreamBufferReset(xStreamBufferRx);
        num++;
        // 1秒待機
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}
