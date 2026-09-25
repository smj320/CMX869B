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

### 割込ハンドラ

```c++
//MODEM_INTは受信のみでOK
//UARTの受信ハンドラの中で
    //UART2のデータ取得
    uint8_t rx_data = (uint8_t)(huart2.Instance->RDR);
    //CS=0
    HAL_GPIO_WritePin(MODEM_CS_GPIO_Port, MODEM_CS_Pin, RESET);
    //アドレス送信
    *(__IO uint8_t *)&hspi1.Instance->DR = (uint8_t)0xE1;
    //データ送信
    *(__IO uint8_t *)&hspi1.Instance->DR = rx_data;
    //CS=1
    HAL_GPIO_WritePin(MODEM_CS_GPIO_Port, MODEM_CS_Pin, SET);

//MODEM_INTの割込ハンドラの中
    //CS=0
    HAL_GPIO_WritePin(MODEM_CS_GPIO_Port, MODEM_CS_Pin, RESET);
    //アドレス送信
    *(__IO uint8_t *)&hspi1.Instance->DR = 0xE4; 
    //受信クロック送信
    *(__IO uint8_t *)&hspi1.Instance->DR = 0xFF; 
    //アドレス送信時のゴミ掃除
    (void)hspi1.Instance->DR;
    //SPIデータ取得
    uint8_t modem_data = *(__IO uint8_t *)&hspi1.Instance->DR;
    //CS=1
    HAL_GPIO_WritePin(MODEM_CS_GPIO_Port, MODEM_CS_Pin, RESET);
    //UARTに送信
    *(__IO uint8_t *)&huart2.Instance->DR = modem_data;

```

ユーザが実装すべき関数は__weakで定義されているので、再定義する。
ハンドラのテンプレ

```c++
 
/**
  * @brief  外部割り込みが発生した時に呼ばれるコールバック
  * @param  GPIO_Pin: 割り込みが発生したピン番号
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == MODEM_INT_Pin) // MODEM_INTピンからの割り込みか判定
  {
    // ここに処理を書く（例：フラグを立てるなど）
  }
}

/**
  * @brief  UART受信が完了した時に呼ばれるコールバック
  */
volatile uint8_t UART2_rxBuffer[1];

メインで実行許可
HAL_UART_Receive_IT(&huart2, UART2_rxBuffer, 1);

コールバック関数
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        // 受信したデータ (UART2_rxBuffer[0]) を処理する
        // 例: モデムへ送信
        Send_Data(UART2_rxBuffer[0]);

        // 次の1バイトを待機するために再度割り込みを有効化する
        HAL_UART_Receive_IT(&huart2, UART2_rxBuffer, 1);
    }
}

/**
  * @brief  タイマーの期間が経過した時に呼ばれるコールバック
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM1) // TIM1（HAL Tick用など）の判定
  {
    // 1msごとの処理など
  }
}


```

## ブリッジ

### UART2の受信割込
UART2のrx割込があったら、割込ハンドラの中でspiに送信してしまう

### MODEM_INTのRxReady割込
MODEM_INT割込があったら、割込ハンドラの中でデータを取得してUART2に送信してしまう。
