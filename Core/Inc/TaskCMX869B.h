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
#define TX_Q_SIZE 81
#define RX_Q_SIZE 83

// ***********************************************************
// 変数
// ***********************************************************
#define HDLC_ADDR_HK 0b00
#define HDLC_ADDR_CMD 0b01
#define HDLC_ADDR_IMG 0b10
//
#define HDLC_ST_SINGLE 0b000
#define HDLC_ST_HEAD 0b001
#define HDLC_ST_MIDDLE 0b010
#define HDLC_ST_LAST 0b011
#define HDLC_ST_ABORT 0b111
typedef union {
    uint8_t Byte;
    struct {
        uint8_t addr: 2; //宛先アドレス
        uint8_t status: 3; //000; single 001 head, 010: middle, 011:last, 111 abort
        uint8_t cnt: 3; //シーケンスカウンタ
    } Bits;
} HDLCControlType;

extern QueueHandle_t xQueueTx;
extern QueueHandle_t xQueueRxHK;
extern QueueHandle_t xQueueRxCMD;
extern QueueHandle_t xQueueRxIMG;
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
