# CMX869B

FreeRTOS版

## 送受信バッファの構成

送受信送信キュー

コントロールビット、
    position  head 0b00, body 0b01, tail 0b10 single b11 2bit
    アドレス　3bit
    アボート　1bit
シーケンスカウンタ
データ長
データバッファ80byte


## 重要事項

### リンクエラー対策

FreeRTOSをいれた場合、Cmakefileにリンカスクリプトを明示しないとエラーになる
```aiignore
# project settings
project(CMX869B C CXX ASM)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

# 以下を追加
set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/STM32F303xx_FLASH.ld")
```

### PLL起動エラー対策

OpenOCDのreset-initがPLL(HSI x16)をSYSCLKにしているので、
そのままだとSystemClock_Config()のPLL再設定がHAL_ERRORになる。HSIに戻しておく
```aiignore
HAL_RCC_DeInit();
```

## メモ


### Txのシーケンス

一回書くと、一文字の送信に対して5回割込がでる。実際の信号とも一致。

```aiignore
HK:09
0000:1 0 0 0 開始（おそらくTX=0から１に変わった）
0000:1 1 0 0 送信完了
0000:1 1 1 0 受信レジスタが有効になった
____:1 1 0 0 FSCを受信開始
0000:1 1 1 0 受信レジスタが有効になった
____:1 1 0 0 CRCを受信開始
0000:1 1 1 0 受信レジスタが有効になった

Txの操作
バッファにデータを入れて、割込許可をするとただちにTxDataReady割込が出るので処理させる。
送信データを送りきったところで最後のTxDataReady割込が出るので、そこで割込禁止にする。

```

oops: 3 の中身
ログに出ていない回は、ステータスを読んだが何もしなかった回です。loops: 3 の部分を分けるとこうなります。
回 | ステータス |処理 | 状況
1  | TXD=1 |73 を書く（xprintf で約4ms） | この間に 70 を受信し、IRQN が Low になる
2 | RXD=1 | 70 を読む（xprintf）| この間に送信バッファが空き、TXD=1 → IRQN が Low になる
3 | TXD=1 | 書くデータが無いので何もしない ここで IRQ がクリアされ、PIN=1 になってループを抜ける

### SEGGER_RTT.h

SEGGER_RTT.hがSEGGER_RTT_ConfigDefault.hを読んで、
EGGER_RTT_ConfigDefault.hがEGGER_RTT_Config.hを呼んでいる。

### 割込に関するち注意

Nucleoのデフォルトは立ち上がりなので、立ち下がりエッジに変更しておく。

### Tips
言語注入の背景書が見づらいので消す。
Settings → Editor → Color Scheme → General → Code → Injected language fragment

### Automodemハンドシェーク成功のために
送信バッファを両電源の5Vで、倍率５倍にする。
ケーブルダミーを繋いでのSNRは6
電池で動く回り込みチェッカーのようなものがあるとよいかも。

### 呼出方法
リセットの効果を知りたいので、ドリルのほうから発信するようにしたい。
すると、ドリルがAnswer側ということになる。
1) GSEのモデムを起動またはリセット、受信待機
2) ドリルを起動、またはリセット

### 外部割込関数を書く位置

```aiignore
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_0) // 設定したピン番号に合わせて変更
    {
        // 割り込み発生時の処理（フラグ操作やLEDのトグルなど）
    }
}
```

### タイマの使い分け
RTOSなしの場合、HAL_delay()関数はSysTickを使う。RTOSがあるときは
OSがSysTickを使い、HAL_delay()はCubeMXのSysでTimebase Source
をTIM6とかに割り当てる。周期タスクはTM2とかを割り当てる。

### RTOSを入れたときのデバッガがとまらない問題

OpenOCD標準の target/stm32f3x.cfg は、書き込みに使うRAMの作業領域をデフォルトで 16KB にしています。
F303K8のSRAMは 12KB しかありません。
•
書き込み用のバッファはイメージが大きいほど大きく確保されます。FreeRTOSを入れてイメージが約19KBに増えたため、
バッファが実在しないRAMの範囲まではみ出し、error writing to flash at address 0x08000000 
で失敗していました。FreeRTOSを入れる前は、イメージが小さかったので表に出なかったのだと思います。
修正した内容
st_nucleo_f3.cfg、st_nucleo_f3_ans.cfg、st_nucleo_f3_call.cfg の3つで、
source [find target/stm32f3x.cfg] の前に次の行を追加しました。
set WORKAREASIZE 0x2000
このあと書き込みは Verified OK になり、ボードには現在のビルドが入っています。

### cmakeの修正
```text
file(GLOB_RECURSE SOURCES "Core/*.*" "Drivers/*.*" "Middlewares/*.*")
に
CONFIGURE_DEPENDS を追加する。
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "Core/*.*" "Drivers/*.*" "Middlewares/*.*")
```
