//
// Created by kikuchi on 2026/09/30.
//
#include "CMX869B.h"
#include "xprintf.h"
//---------------------------------------
// 初期化
// 地上側受信　モデムの受信割込をUARTにながす
// 地上側送信  UARTの受信割込でCMXにデータを流す
//---------------------------------------
// 1200bps 半二重
// 発振の確認、割込受信デバッグのループバックとかに使う
extern CMX869B_GRE_TypeDef GRE;
extern CMX869B_TxReg_TypeDef TxReg;
extern CMX869B_RxReg_TypeDef RxReg;
extern CMX869B_QamReg_TypeDef QamReg;
extern CMX869B_StatusReg_TypeDef StatusReg;
extern CMX869B_QamStatusReg_TypeDef QamStatusReg;

void set_bell(void) {
    // TX
    TxReg.Bits.TxMode = TxReg_Mode_BELL;
    TxReg.Bits.StartStop = 0b10; //Start-stop, NonParity
    TxReg.Bits.DataBits = 0b110; //8bit Stop1
    cbus_write(TxReg_ADDR, TxReg.Word);
    // RX
    RxReg.Bits.RxMode = RxReg_Mode_BELL;
    RxReg.Bits.StartStop_Synch = 0b110; //Start-stop, NonOverSpeed
    RxReg.Bits.BitsParity = 0b111; //8bit, NonParity
    cbus_write(RxReg_ADDR, RxReg.Word);
}

// 2400 bps 全二重
// ネゴシエーション不用なので、ブチ切りでもいける。
void set_v22_call(void) {
    // TX
    TxReg.Bits.TxMode = TxReg_Mode_V22_CALL;
    TxReg.Bits.StartStop = 0b10; //Start-stop, NonParity
    TxReg.Bits.DataBits = 0b110; //8bit Stop1
    cbus_write(TxReg_ADDR, TxReg.Word);
    // RX
    RxReg.Bits.RxMode = RxReg_Mode_V22_CALL;
    RxReg.Bits.StartStop_Synch = 0b110; //Start-stop, NonOverSpeed
    RxReg.Bits.BitsParity = 0b111; //8bit, NonParity
    cbus_write(RxReg_ADDR, RxReg.Word);
}

void set_v22_ans(void) {
    // TX
    TxReg.Bits.TxMode = TxReg_Mode_V22_ANS;
    TxReg.Bits.StartStop = 0b10; //Start-stop, NonParity
    TxReg.Bits.DataBits = 0b110; //8bit Stop1
    cbus_write(TxReg_ADDR, TxReg.Word);
    // RX
    RxReg.Bits.RxMode = RxReg_Mode_V22_ANS;
    RxReg.Bits.StartStop_Synch = 0b110; //Start-stop, NonOverSpeed
    RxReg.Bits.BitsParity = 0b111; //8bit, NonParity
    cbus_write(RxReg_ADDR, RxReg.Word);
}

void set_v22_loop(void) {
    // TX
    TxReg.Bits.TxMode = TxReg_Mode_V22_ANS;
    TxReg.Bits.StartStop = 0b10; //Start-stop, NonParity
    TxReg.Bits.DataBits = 0b110; //8bit Stop1
    cbus_write(TxReg_ADDR, TxReg.Word);
    // RX
    RxReg.Bits.RxMode = RxReg_Mode_V22_CALL;
    RxReg.Bits.StartStop_Synch = 0b110; //Start-stop, NonOverSpeed
    RxReg.Bits.BitsParity = 0b111; //8bit, NonParity
    cbus_write(RxReg_ADDR, RxReg.Word);
}

// Auto modem
void set_qam_answer(void) {
    // TX
    TxReg.Bits.TxMode = TxReg_Mode_QAM_AUTO;
    TxReg.Bits.StartStop = 0b10; //Start-stop, NonParity
    TxReg.Bits.DataBits = 0b110; //8bit Stop1
    cbus_write(TxReg_ADDR, TxReg.Word);
    // RX
    RxReg.Bits.RxMode = RxReg_Mode_QAM_AUTO;
    RxReg.Bits.StartStop_Synch = 0b110; //Start-stop, NonOverSpeed
    RxReg.Bits.BitsParity = 0b111; //8bit, NonParity
    cbus_write(RxReg_ADDR, RxReg.Word);
    //QAM
    QamReg.Bits.br = QamMaxBR_14400;
    QamReg.Bits.command = QamAnswer;
    QamReg.Bits.zeros = QamAZeros;
    cbus_write(QamCmdReg_ADDR, QamReg.Word);
    /*
    for (int i = 0; i < 200; i++) {
        //receive_qam_status(&QamStatusReg);
        //xprintf("%02x, %02x\r\n",QamStatusReg.Word[0],QamStatusReg.Word[1]);
    };
    */
}

void set_qam_call(void) {
    // TX
    TxReg.Bits.TxMode = TxReg_Mode_QAM_AUTO;
    TxReg.Bits.StartStop = 0b10; //Start-stop, NonParity
    TxReg.Bits.DataBits = 0b110; //8bit Stop1
    cbus_write(TxReg_ADDR, TxReg.Word);
    // RX
    RxReg.Bits.RxMode = RxReg_Mode_QAM_AUTO;
    RxReg.Bits.StartStop_Synch = 0b110; //Start-stop, NonOverSpeed
    RxReg.Bits.BitsParity = 0b111; //8bit, NonParity
    cbus_write(RxReg_ADDR, RxReg.Word);
    //QAM
    QamReg.Bits.br = QamMaxBR_14400;
    QamReg.Bits.command = QamCall;
    QamReg.Bits.zeros = QamAZeros;
    cbus_write(QamCmdReg_ADDR, QamReg.Word);
}

int get_qam_itm2(uint16_t mode) {
    int bps = 0;
    int tim2 = 0;
    switch (mode) {
        case 0b1111: //14400
            bps = 14400;
            break;
        case 0b1110: //12000
            bps = 12000;
            break;
        case 0b1101: //9600
        case 0b1100:
            bps = 9600;
            break;
        case 0b1011: //7200
            bps = 7200;
            break;
        case 0b1010: //4800
            bps = 4800;
            break;
        case 0b1001: //2400
            bps = 2400;
            break;
        default:
            bps = 1200;
            break;
    }
    tim2 = 100*1200/bps-1;
    return tim2;
}
