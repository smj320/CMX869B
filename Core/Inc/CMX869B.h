//
// Created by kikuchi on 2026/01/12.
// Xtal 6.144MHz, 47pF
//
#ifndef CMX869B_CMX869B_H
#define CMX869B_CMX869B_H

#include "main.h"
#include <stdint.h>


// 割り当てによって適宜変更,ソースでは無番号を使う
extern UART_HandleTypeDef huart2;
extern SPI_HandleTypeDef hspi1;
#define huart huart2
#define hspi hspi1

// 外部公開用
void CMX869B_Init(int is_gse);

#define CMX_TX_BUFFER 10
#define CMX_RX_BUFFER 10

void CMX869B_read_data(uint8_t data[], int len);

void CMX869B_write_data(uint8_t data[], int len);

// 割込関数
// mainのvoid HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)の中で
// if (GPIO_Pin == C_INT_Pin) CMX869B_RCV_INT(); と登録
void CMX869B_RCV_INT(void);

// xprintf用
//mainの初期化のところで xdev_out(uart_putc); と登録
void uart_putc(unsigned char c);

#endif //CMX869B_CMX869B_H
