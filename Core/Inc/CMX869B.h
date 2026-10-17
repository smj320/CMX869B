//
// Created by kikuchi on 2026/01/12.
// Xtal 6.144MHz, 47pF
//
#ifndef CMX869B_CMX869B_H
#define CMX869B_CMX869B_H

#include "main.h"
#include <stdint.h>
//
extern UART_HandleTypeDef huart2;
extern SPI_HandleTypeDef  hspi1;
//
void CMX869B_Init(int is_gse);
void CMX869B_read(const uint8_t data[], int len);
void CMX869B_write(const uint8_t data[], int len);
void CMX869B_RCV_INT(void);
//
void uart_putc(unsigned char c);

#endif //CMX869B_CMX869B_H