//
// Created by kikuchi on 2026/10/23.
//

#ifndef CMX869B_TASKCMX869B_H
#define CMX869B_TASKCMX869B_H

#include "cmsis_os.h"
#include "stream_buffer.h"

extern StreamBufferHandle_t xStreamBufferTx;
extern StreamBufferHandle_t xStreamBufferRx;

void TaskCMX869BEntry(void *argument);
void INT_CMX869B_ISR(void);
void CMX869B_putc(uint8_t tx_data);
void CMX869B_flush_Tx();
void CMX869B_get_qam_status(uint8_t *mode, uint8_t *snr, uint8_t *msg);

void uart_putc(unsigned char c);

#endif //CMX869B_TASKCMX869B_H
