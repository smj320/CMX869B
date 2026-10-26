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
#include "TaskCMX869B_priv.h"

void TaskHKEntry(void *argument) {
    uint8_t rx_buf[RX_Q_SIZE] = {0};

    // 基準時刻の初期化と現在のティック数を取得
    const TickType_t xFrequency = pdMS_TO_TICKS(1000*2); // 1000msをTick数に変換
    TickType_t xLastWakeTime = xTaskGetTickCount();

    HDLCControlType hdlc_control;

    int cnt = 0;
    int f_error = 0;
    uint8_t tx_data[TX_Q_SIZE] = {0};
    for (;;) {
        if (Is_gse_mode == 0) {
            // 処理開始
            xprintf("%04d HK:\r\n", cnt);
            // データを編集する。
            hdlc_control.Bits.addr = HDLC_ADDR_HK;
            hdlc_control.Bits.status = HDLC_ST_SINGLE;
            hdlc_control.Bits.cnt = cnt&0b111;
            tx_data[0] = hdlc_control.Byte;
            for (int i = 1; i < TX_Q_SIZE; i++) {
                tx_data[i] = cnt;
            }
            xQueueSend(xQueueTx, tx_data, 0);
            CMX869B_flush_Tx();
        }else {
            // 読む
            if (xQueueReceive(xQueueRxHK, rx_buf, pdMS_TO_TICKS(1500))==pdPASS){
                if (((HDLCControlType *)&rx_buf[0])->Bits.status == HDLC_ST_ABORT) {
                    f_error++;
                }
                for (int i = 0; i < RX_Q_SIZE; i++) {
                    xprintf("%02X ", rx_buf[i]);
                }
                xprintf("\r\n");
            }else {
                xprintf("TimeOut\r\n");
            }
        }
        // 回線状態の表示(受信バイト数とQAMステータス)
        uint8_t mode, snr, msg;
        CMX869B_get_qam_status(&mode, &snr, &msg);
        xprintf("%04d FE:%03d SNR:%d MODE:%02X MSG:%02X\r\n", cnt, f_error, snr, mode, msg);
        // 1秒待機
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        cnt++;
    }
}
