# モデムの動き

地上(ジャンパあり)、地下(ジャンパなし)
内部Pull UPしているので、地上はLOW, ドリルはHiになる。
ジャンパあり(Low)はCalling
ジャンパなし(Hi)はAnswer

## 地上系

接続ボタンを押すと,
モデムリセット
Ringパルス出力
    相手をAnswer状態に入れる
モデム待機状態
    Ring出力後受信状態に入れる
ハンドシェーク

## ドリル系

Ringが来たらモデムリセットに入る（ボタンで代用）。
    バンドパスフィルタ+整流？
ハンドシェークが終わったら、フレームを出力する。

## GPIOのデータを読む
if(1) 真
