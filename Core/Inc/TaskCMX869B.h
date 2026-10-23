//
// Created by kikuchi on 2026/10/23.
//

#ifndef CMX869B_TASKCMX869B_H
#define CMX869B_TASKCMX869B_H

void TaskCMX869BEntry(void *argument);
void INT_CMX869B_ISR(void);
void uart_putc(unsigned char c);

#endif //CMX869B_TASKCMX869B_H
