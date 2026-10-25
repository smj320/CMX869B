//
// Created by kikuchi on 2026/10/23.
//

#ifndef CMX869B_TASKCMX869B_H
#define CMX869B_TASKCMX869B_H

#include "cmsis_os.h"
#include "queue.h"

// ***********************************************************
// 各種バッファサイズ
// ***********************************************************
#define CMX_SERVICE_MAX_LOOP 8
#define RX_BUF_SIZE 10
#define TX_BUF_SIZE 10

// ***********************************************************
// 変数
// ***********************************************************
extern QueueHandle_t xTxQueue;
extern QueueHandle_t xRxQueue;
extern int Is_gse_mode;

// ***********************************************************
// 関数
// ***********************************************************
void TaskCMX869BEntry(void *argument);
void INT_CMX869B_ISR(void);
void CMX869B_putc(uint8_t tx_data);
void CMX869B_flush_Tx();
void CMX869B_get_qam_status(uint8_t *mode, uint8_t *snr, uint8_t *msg);

void uart_putc(unsigned char c);

#endif //CMX869B_TASKCMX869B_H
