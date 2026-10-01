# CMX869B

## 気付き事項

### 割込設定
ピンの割込を有効にすると、
__weak void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
というのがstm32xxx_hal_gpio.hの中に生成される。これをオーバーライドする。

ピンをディスパッチする必要があるので、main.cにかくのがよいか。

```aiignore
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_0) // 設定したピン番号に合わせて変更
    {
        // 割り込み発生時の処理（フラグ操作やLEDのトグルなど）
    }
}
```


### ループ待機
SPIのクロックは4MHzになっている。10クロック分で400kHZ,2.5usec
forループでビットを落としたあと上がるまでの時間は2usec
5回で9usec
10回で15usec
1000回で6msec
10009回で18msec
forループで2回せばタイムアウトは避けられる。

GPIOで作ったクロックの間隔は5usec, 1ワード50usec
ペリフェラルクロックを４倍に挙げると１ワード15usec

最初の１回はゴミデータが入っていて、RxDataRadyになるが、それ以降は
データが入ってこなければRxDataReadyにはならないようだ。

### 送信データ
何もしない状態で割込を許可しても勝手に割込は始まらない。

### ネイティブアクセス
*((__IO uint8_t *)&hspi->Instance->DR) = (*hspi->pTxBuffPtr);

## モデムへの流入経路

1200bpsの場合は1文字10bitなので1/120=8msecの間隔が必要だが少し広げて10msecにすると安定する。

## タイマの用途
RTOSなしの場合、HAL_delay()関数はSysTickを使う。RTOSがあるときは
OSがSysTickを使い、HAL_delay()はCubeMXのSysでTimebase Source
をTIM6とかに割り当てる。周期タスクはTM2とかを割り当てる。
周期動作をした場合の割込ハンドラはmain.cの
HAL_TIM_PeriodElapsedCallback
にかかれる。タイマを判別する必要がある。

### 送信
ボーレートよりちょっと遅めのポーリングで送信

### 受信
受信割込で。

## RTOSを入れたときのデバッガがとまらない問題

OpenOCD標準の target/stm32f3x.cfg は、書き込みに使うRAMの作業領域をデフォルトで 16KB にしています。F303K8のSRAMは 12KB しかありません。
•
書き込み用のバッファはイメージが大きいほど大きく確保されます。FreeRTOSを入れてイメージが約19KBに増えたため、バッファが実在しないRAMの範囲まではみ出し、error writing to flash at address 0x08000000 で失敗していました。FreeRTOSを入れる前は、イメージが小さかったので表に出なかったのだと思います。
修正した内容
st_nucleo_f3.cfg、st_nucleo_f3_ans.cfg、st_nucleo_f3_call.cfg の3つで、source [find target/stm32f3x.cfg] の前に次の行を追加しました。
set WORKAREASIZE 0x2000
このあと書き込みは Verified OK になり、ボードには現在のビルドが入っています。

## ベアメタル風SPIのアクセス

```C
// ※hspi1 は CubeMX が生成した SPI_HandleTypeDef のインスタンスと仮定

// 送信レジスタ（TXE: Transmit buffer Empty）が空か確認して1文字投げる
if ((hspi1.Instance->SR & SPI_FLAG_TXE) && !IsTxFifoEmpty())
{
    uint8_t tx_data = PopTxFifo();
    // データレジスタ（DR）に書き込むだけで送信が走る
    *(__IO uint8_t *)(&hspi1.Instance->DR) = tx_data; 
}

// 受信レジスタ（RXNE: Receive buffer Not Empty）にデータがあるか確認して読む
if (hspi1.Instance->SR & SPI_FLAG_RXNE)
{
    uint8_t rx_data = *(__IO uint8_t *)(&hspi1.Instance->DR);
    PushRxFifo(rx_data);
}
```

### cmakeの修正
```text
file(GLOB_RECURSE SOURCES "Core/*.*" "Drivers/*.*" "Middlewares/*.*")
に
CONFIGURE_DEPENDS を追加する。
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "Core/*.*" "Drivers/*.*" "Middlewares/*.*")
```
