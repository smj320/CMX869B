//
// Created by kikuchi on 2026/10/23.
//

#ifndef CMX869B_TASKCMX869B_H
#define CMX869B_TASKCMX869B_H

#include "cmsis_os.h"
#include "stream_buffer.h"

void TaskCMX869BEntry(void *argument);
void INT_CMX869B_ISR(void);
void CMX869B_flush_Tx(char c);
void uart_putc(unsigned char c);

extern StreamBufferHandle_t xStreamBufferTx;
extern StreamBufferHandle_t xStreamBufferRx;

#endif //CMX869B_TASKCMX869B_H
