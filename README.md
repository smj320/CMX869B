# CMX869B

## 気付き事項

### 送信データ
何もしない状態で割込を許可しても勝手に割込は始まらない。

### ネイティブアクセス
*((__IO uint8_t *)&hspi->Instance->DR) = (*hspi->pTxBuffPtr);


## モデムへの流入経路

地上系
UART2のRXDにデータがあれば、CMX869bのTXDに書き込む
CMX869bのRXDにデータがあれば、UART2のTXDに書き込む

ドリル系
ドリルはTXD_FIFOにデータがあればCMX869bのTXDに書き込む
CMX869bのRXDにデータがあれば、コマンドデコードを行う

## タイマの用途
RTOSなしの場合、HAL_delay()関数はSysTickを使う。RTOSがあるときは
OSがSysTickを使い、HAL_delay()はCubeMXのSysでTimebase Source
をTIM6とかに割り当てる。周期タスクはTM2とかを割り当てる。
周期動作をした場合の割込ハンドラはmain.cの
HAL_TIM_PeriodElapsedCallback
にかかれる。タイマを判別する必要がある。


## 脇込みハンドラの再定義

HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
のようなものはcubemxが __weak属性で定義してくれるので、
ユーザーが同じ名前で定義するとリンクで上書きできる。

## 操作受信

基本、送受信は割込でやりたい。TX,RXともに割込の
禁止は行わず、内容によって無視することで対応

## レジスタの反応

GREのIRQ_MASKのb3をオンにすると、
・TXバッファがReadyのき割込が起こる。TXDにデータを書くとクリアされる
この要因で発生しているときは、StatuRegのb12が1
・TXバッファが送信終了になると割込が起こる。TXDにデータを書くとクリアされる
この要因で発生しているときは、StatuRegのb11が1

GREのIRQ_MASKのb0をオンにすると、
・RXバッファがReadyのき割込が起こる。RXDを読むとクリアされる
　この要因で発生しているときは、StatuRegのb6が1
・RXバッファが玉突きで割込が起こる。RXDを読むとクリアされる
　この要因で発生しているときは、StatuRegのb5が1

ステータスレジスタを読むと割込線は一旦復旧するが、要因が
残っている場合は再度アクティブになる。

### 送信

バッファにデータをセットして、割込を許可
送信バッファがReadyになっているので、ただちに割込が発生
1文字送信してlen--
len=1の場合は最後の文字なので、割込マスクを落とす

### 受信

ステートを0にして受信、0なら廃棄
ヘッダならステートを1にしてバッファに保存
ステートが1のうちはバッファに保存
バッファオーバランになったらステートを0に戻して待機
クローズが来たらバッファを解析して実行、ステートを0に戻す


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

