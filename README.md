# CMX869B

##  テンプレートファイル

```aiignore
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Core/*.*"
    "${CMAKE_CURRENT_SOURCE_DIR}/Drivers/*.*"
    "${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/*.*"
)
```
のようにすると、IDEが正しく認識してくれるようになる

## 割込に関するち注意

デフォルトは立ち上がりなので、立ち下がりエッジに変更しておく。
データを書き込むときに割込が起こるとCBUSが取られるので、最初の１個を書くときは割込を禁止すべき


## 割込の中でのSPI送信

```aiignore
// 1. データを送信バッファ（DR）に書き込む
// （16bitモードなら hspi1.Instance->DR = data; など）
*(volatile uint8_t*)&(hspi1.Instance->DR) = tx_data; 

// 2. 送信バッファが空（TXE = 1）になるのを待つ（必要に応じて）
// または、すぐに次のデータを書かないなら省略してBSYだけでもOKですが、
// 一般的にはTXEが立つのを確認してから次の処理へ進みます。
while (!(hspi1.Instance->SR & SPI_SR_TXE)) {
    // 必要ならタイムアウトカウンタを入れるが、
    // 8Mbpsなら数クロック〜数十クロックですぐに抜けるため通常はループだけでOK
}

// 3. 【最も重要】通信の完全終了（BSYが0になる）を待つ
// これを待たないと、まだシフトアウトの途中でCSをHに戻してしまう恐れがあります。
while (hspi1.Instance->SR & SPI_SR_BSY) {
    // BSYがクリアされる（通信が物理的に完了する）のを待つ
}
```
## 割込の中でのSPI受信

```aiignore
// 1. ダミーデータ（または送信したいコマンド）をDRに書いてクロックを発生させる
*(volatile uint8_t*)&(hspi1.Instance->DR) = tx_data; 

// 2. 送信が完了し、受信バッファに新しいデータが入る（RXNEフラグが1になる）のを待つ
while (!(hspi1.Instance->SR & SPI_SR_RXNE)) {
    // 受信完了待ち（8Mbpsなら1µs足らずですぐ抜けます）
}

// 3. 受信データレジスタ（DR）から読み取る
uint8_t rx_data = *(volatile uint8_t*)&(hspi1.Instance->DR);
```

## 送信後のステータス

ループバックでの送信で、1msecごとにステータスレジスタを見ると、
送信直後、送信レジスタが空いていないときはTxRdy=0, TxUdr=0
送信レジスタが空いて,物理的な送信も完了するとTxUnd=1になる。
その後RxRdyが1になる。

```aiignore
05 TxRdy:0 TxUnd:0 RxRdy:0 RxOvr:0
05 TxRdy:1 TxUnd:0 RxRdy:0 RxOvr:0
05 TxRdy:1 TxUnd:0 RxRdy:0 RxOvr:0
05 TxRdy:1 TxUnd:0 RxRdy:0 RxOvr:0
05 TxRdy:1 TxUnd:0 RxRdy:0 RxOvr:0
05 TxRdy:1 TxUnd:1 RxRdy:0 RxOvr:0
05 TxRdy:1 TxUnd:1 RxRdy:0 RxOvr:0
05 TxRdy:1 TxUnd:1 RxRdy:0 RxOvr:0
05 TxRdy:1 TxUnd:1 RxRdy:1 RxOvr:0
05 TxRdy:1 TxUnd:1 RxRdy:1 RxOvr:0
05 TxRdy:1 TxUnd:1 RxRdy:1 RxOvr:0
05 TxRdy:1 TxUnd:1 RxRdy:1 RxOvr:0
05 TxRdy:1 TxUnd:1 RxRdy:1 RxOvr:0
05 TxRdy:1 TxUnd:1 RxRdy:1 RxOvr:0
05 TxRdy:1 TxUnd:1 RxRdy:1 RxOvr:0
05 TxRdy:1 TxUnd:1 RxRdy:1 RxOvr:0
05 TxRdy:1 TxUnd:1 RxRdy:1 RxOvr:0
05 TxRdy:1 TxUnd:1 RxRdy:1 RxOvr:0
05 TxRdy:1 TxUnd:1 RxRdy:1 RxOvr:0
05 TxRdy:1 TxUnd:1 RxRdy:1 RxOvr:0
```

割込線は1になるとLOWになるという単純なもの。エッジ検出にするか
どうかはCPU側で決まる。

ステータスレジスタを読むと、割込線は一旦元に戻る。
戻った後は、次の割込要因のビット変動がないと再割込はかからない。

RxOverflowが出た場合は、上書きされてしまっているので、一つ前の
ビットは抜けることになる。

HDLCで送受信するなら、パケットレベルのCRCエラーになる。

### tips
言語注入の背景書が見づらいので消す。
Settings → Editor → Color Scheme → General → Code → Injected language fragment

## グルー関数について

```aiignore
embedded（組み込み）の世界で「車輪の再発明」を避けてうまく既存の仕組みに乗っかるための、
定番アプローチをいくつか整理してみました。

1. lwIP公式のひな形（ethernetif.c）を改造する
lwIPのソースツリー（contrib フォルダなど）には、ドライバのテンプレートである 
ethernetif.c が用意されています。

これをそのまま使うのは無理でも、「lwIPの pbuf とハードウェアドライバをどう繋いでいるか」
の公式のリファレンス実装として非常に参考になります。

自前のHDLCドライバ層も、このテンプレートの骨組み（初期化、linkoutput、
入力スレッドの構造）をそのまま流用・リファクタリングして作ると、車輪の再発明をかなり回避できます。

2. 自前でリングバッファを書かず、FreeRTOSのプリミティブを信頼する
バイト単位やフレーム単位のやり取りでバッファが必要になったとき、配列とインデックスを
自分で管理するバグだらけのリングバッファを書きがちですが、FreeRTOSの標準機能を使うのが安全です。

StreamBuffer / MessageBuffer:
FreeRTOSには、可変長メッセージやバイトストリームを安全にやり取りするための優秀な
バッファ機能が標準で備わっています（内部の排他制御も完璧に作り込まれています）。

これをハードウェア割込と受信タスクの間の「安全なパイプ」として使えば、バッファ管理の苦労から解放されます。

3. プロトコル層の選択（SLIPやPPPという先人たちの知恵）
もしCMX869BのHDLC経由でIPパケットを流す場合、フル規格のイーサネットフレームを模倣するだけでなく、
lwIPが標準で持っているシリアル系プロトコルを流用するという手もあります。

lwIPには SLIP (Serial Line Internet Protocol) や PPP といった、
シリアル回線上でIPパケットを通すための枯れたモジュールが最初から入っています。

「HDLCのフレーミングはあるけれど、Ethernetヘッダー（MACアドレスなど）
の処理を省いてIPパケットだけダイレクトに流したい」という要件であれば、
SLIPの仕組みやパケット構造のハンドリング方法がそのまま大きなヒントになります。

今回の構成（CMX869B ＋ FreeRTOS ＋ lwIP）において、一番手間の少ない「美味しいところ取り」
をするなら、ハードウェアとの送受信バッファにはFreeRTOSのMessageBufferを使い、
lwIP側との結合には公式の netif 構造体の作法（linkoutput と input）
を最小限のコードで実装する という形が一番手堅そうですね。

ちなみに、今回のハードウェア構成（CMX869Bを用いた南極のドリル監視などの通信サブシステム）では、
ドライバ層は自作しつつ、OSやプロトコルスタックの部分はどのあたりまで既存のものを組み込む予定で進められていますか？
```


## Automodemハンドシェーク成功
送信バッファを両電源の5Vで、倍率５倍にする。
ケーブルダミーを繋いでのSNRは6
電池で動く回り込みチェッカーのようなものがあるとよいかも。

## 気付き事項

### 呼び方
リセットの効果を知りたいので、ドリルのほうから発信するようにしたい。
すると、ドリルがAnswer側ということになる。
1) GSEのモデムを起動またはリセット、受信待機
2) ドリルを起動、またはリセット


### 割込の変換
対応するレジスタが「0から1に変化したとき」マスクが空いていれば割込線を落とす。
割込線自体は、StatusRegisterを読むとHに戻る。

TxDataReady=1 →TxDataを書いてよければ１になる。TxDataAddrに書くと０になる。
TxDataUnderflow=1 TxDataを書き込まないと1になる。TxDataAddrに書くと０になる。

### ステータス判定
#### TXの場合
TxDataReadyは次のTxDataを受け付けることができるフラグで、一つ送信した後
10msec以内にOnになる。 一方TxDataUnderflowフラグは変調信号を送り終わった
タイミングはで上がる。
なので連続送信を行う場合、次の送信タイミングはTxDataUnderflow=1となる。
ただし送信にあたってはTxDataReady=1を確認する必要がある。

TIM2の周期割込でやる場合、1200bpsで80byteの場合
プリスケーラ799, カウント99だとで丁度良い



#### RXの場合
ループバックにしたとき、変調信号が出きって信号の1.5倍くらいのところで
TxDataOverflow=1が立つ。このタイミングで読むべき。

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
