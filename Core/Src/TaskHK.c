//
// Created by kikuchi on 2026/10/21.
//
#include "main.h"
#include "cmsis_os.h"
#include "xprintf.h"

void TaskHKEntry(void *argument) {
    for (;;) {
        xprintf("HK: Frank Zap\r\n");
        osDelay(1000);
    }
}