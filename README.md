# モデムの動き

地上系をリセットして、受信待機にする。
ドリル系をリセットして、キャリアを出させる。
地上(キャリアを出す側、ジャンパあり)
地下(キャリアを受ける側、ジャンパなし)

## GPIOのデータを読む

HAL_GPIO_ReadPin()は、落ちていれば0で、立っているときは0以外。
HAL_GPIO_ReadPin()　== GPIO_RESET　0
HAL_GPIO_ReadPin()　!= GPIO_RESET　1を含む0以外
で判定する。

## openocdのconfigパス

/opt/homebrew/Cellar/open-ocd/0.12.0_1/share/openocd/scripts/interface/stlink.cfg

```
adapter driver hla
hla_layout stlink
hla_device_desc "ST-LINK"
hla_vid_pid 0x0483 0x3744 0x0483 0x3748 0x0483 0x374b 0x0483 0x374d 0x0483 0x374e 0x0483 0x374f 0x0483 0x3752 0x0483 0x3753 0x0483 0x3754
```
とあるので、ここに
```
hla_serial 12345623498723497
```
とlsusbで確認したデバッガのシリアル番号を指定する。

## VCでのデバッグ
設定は .vscode/launch.jsonに書く

```angular2html
    "version": "0.2.0",
    "configurations": [
          {
            "cwd": "${workspaceRoot}",
            "executable": "./cmake-build-debug/CMX869B.elf",
            "name": "Debug Microcontroller",
            "device": "STM32F303",
            "runToMain": true,
            "request": "launch",
            "type": "cortex-debug",
            "servertype": "openocd",
            "gdbTarget": "localhost:3333",
            "configFiles": [
                "./st_nucleo_f3_1.cfg"
            ],
        }
    ]
```
