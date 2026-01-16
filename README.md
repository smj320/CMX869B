# CMX869B

## IRQマスク

### 共通

| Status | IRQ |            コメント |
|:-------|:---:|----------------:|
| b15    | b6  |             IRQ |
| b14    | b5  |     Ring Detect |
| b13    | b4  |      Programing |
| b12    | b3  |    TxData Ready |
| b11    | b3  | TxData Overflow |

### Rx Manual Modem

| Status | IRQ |              コメント |
|:-------|:---:|------------------:|
| b10    | b2  |     Energy Detect |
| b9     | b1  | Frame Sync Detect |
| b8     | b1  |               右寄せ |

### All Modem

| Status | IRQ |            コメント |
|:-------|:---:|----------------:|
| b15    | b0  |    RxData Ready |
| b6     | b0  | RxData Overflow |

imask = 0b001001