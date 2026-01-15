//
// Created by kikuchi on 2026/01/12.
//

#ifndef CMX869B_CMX869B_H
#define CMX869B_CMX869B_H

/*
 * Xtal 6.144MHz, 47pF
 */
#include <stdint.h>

//-------------------------------------
// レジスタ
// ビットフィールドは上がb0で下がb15
// 送信するときはByte[1],Byte[0]の順
//-------------------------------------
#define General_Reset 0x01

#define GRE_ADDR 0xE0
typedef union {
    uint8_t  Bytes[2];
    struct {
        uint16_t IrqMask : 6; //b0
        uint16_t IrqEna : 1;
        uint16_t Rst : 1;
        uint16_t Pwr : 1;
        uint16_t RlyDrv : 1;
        uint16_t _2C : 1;
        uint16_t LB : 1;
        uint16_t  Zero : 1;
        uint16_t  PatDet : 1;
        uint16_t  HighGain : 1;
        uint16_t  Equ : 1;
    } __attribute__((packed)) Bits;
} CMX869B_GRE_TypeDef;

#define TxReg_ADDR 0xE1
#define TxReg_Mode_V22_AUTO 0b1111
#define TxReg_Mode_V22_ANS 0b1011
#define TxReg_Mode_V22_CALL 0b1010
typedef union {
    uint8_t  Bytes[2];
    struct {
        uint16_t DataBits : 3; //b0
        uint16_t StartStop : 2;
        uint16_t Scramble : 2;
        uint16_t GardTone : 2;
        uint16_t TxLevel : 3;
        uint16_t TxMode : 4;
    } __attribute__((packed)) Bits;
} CMX869B_TxReg_TypeDef;

#define RxReg_ADDR 0xE2
typedef union {
    uint8_t  Bytes[2];
    struct {
        uint16_t BitsParity : 3; //b0
        uint16_t StartStop_Synch : 3;
        uint16_t Equaliser : 3;
        uint16_t RxLevel : 3;
        uint16_t RxMode : 4;
    } __attribute__((packed)) Bits;
} CMX869B_RxReg_TypeDef;

#define QamStatusReg_ADDR 0xEB
typedef union {
    uint8_t  Bytes[2];
    struct {
        uint16_t Mode : 4; //b0
        uint16_t SNR : 3;
        uint16_t Zeros : 3;
        uint16_t Messages : 6;
    } __attribute__((packed)) Bits;
} CMX869B_QamStatusReg_TypeDef;

#define TxData_ADDR 0xE3   //8bit
#define RxData_ADDR 0xE5   //8bit

#define StatusReg_ADDR 0xE6
typedef union {
    uint8_t  Bytes[2];
    struct {
        uint16_t FSKDemodulatorOutput : 1; //b0
        uint16_t TwoCharacter : 1;
        uint16_t HDLC_FSC_ABORT: 1;
        uint16_t HDLC_FSC_RCV : 1; //3
        uint16_t FcsError : 1; //4
        uint16_t RxDataOverflow : 1;
        uint16_t RxDataReady : 1; //6
        uint16_t Zero2 : 2;
        uint16_t ModemEvent : 1;
        uint16_t Zero1 : 1; //10:
        uint16_t TxU : 1;
        uint16_t TxD : 1;
        uint16_t PF : 1;
        uint16_t RD : 1;
        uint16_t IRQ : 1; //15
    } __attribute__((packed)) Bits;
} CMX869B_StatusReg_TypeDef;


#define QamReg_ADDR 0xEA
//
#define QamMaxBR_14400 0x111
#define QamMaxBR_12000 0x110
#define QamMaxBR_9600 0x101
#define QamCall 0x010
#define QamAnswer 0x011
typedef union {
    uint8_t  Bytes[2];
    struct {
        uint16_t protocol : 3;
        uint16_t command : 3; //Calling/Answer
        uint16_t zeros : 13;
    }  __attribute__((packed)) Bits;
} CMX869B_QamReg_TypeDef;

#define QamStatusReg_ADDR 0xEB
typedef union {
    uint8_t  Bytes[2];
    struct {
        uint16_t Mode : 4;
        uint16_t snr : 3;
        uint16_t zeros : 3;
        uint16_t message : 6;
    } __attribute__((packed)) Bits;
} CMX869B_QamStatus_TypeDef;

void CMX869B_Init(void);

#endif //CMX869B_CMX869B_H