//
// Created by kikuchi on 2026/10/21.
//

#ifndef CMX869B_MODEMTASK_H
#define CMX869B_MODEMTASK_H


#include "main.h"
#include <stdint.h>


// 割り当てによって適宜変更,ソースでは無番号を使う
extern UART_HandleTypeDef huart2;
extern SPI_HandleTypeDef hspi1;
extern TIM_HandleTypeDef htim2;
#define huart huart2
#define hspi hspi1
#define htim htim2

typedef union {
    uint16_t Word;
    struct {
        uint16_t FSKDemodulatorOutput: 1; // b0
        uint16_t Char2Mode: 1; // b1
        uint16_t Char2EvenHasParity: 1; // b2
        uint16_t FCS: 1; // b3
        uint16_t FrameError: 1; // b4
        uint16_t RxDataOverflow: 1; // b5　 MASK b00
        uint16_t RxDataReady: 1; // b6 　MASK b00
        uint16_t b7_Unused: 1; // b7
        uint16_t b8_RightJustify: 1; // b8
        uint16_t b9_FrameSyncDetect: 1; // b9
        uint16_t EnergyDetect: 1; // b10
        uint16_t TxDataUnderflow: 1; // b11　MASK b03
        uint16_t TxDataReady: 1; // b12　MASK b03
        uint16_t ProgrammingFlag: 1; // b13
        uint16_t RingDetect: 1; // b14
        uint16_t IRQ: 1; // b15
    } __attribute__((packed)) Bits;
} CMX869B_Status;


// 外部公開用
//#define N_TXBuf 1700
//#define N_RXBuf 1700
//#define N_TXBuf 80
//#define N_RXBuf 80
#define N_TXBuf 80
#define N_RXBuf 80
//
void CMX_putc(char c);
uint8_t CMX_getc();
void Init(int is_gse);// xprintf用
//mainの初期化のところで xdev_out(uart_putc); と登録
void uart_putc(unsigned char c);


#endif //CMX869B_MODEMTASK_H
