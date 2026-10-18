# CMX869B

ベアメタル版。

メインルーチンからAPIでバッファのデータを書き込むと、割込でHDLCフレーム送信。
受信データはHDLCのFCSフラグを見てバッファに溜めて、メインルーチンからAPIで引き抜く。
FreeOS+lwIPでソケットAPIをつかったり自動再送させるためのベース。

## メモ

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
