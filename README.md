# WT32-SC01-Plus_LVGL-HardwareTest
WT32-SC01 Plus を使用したハードウェア検証の調査および開発をしています。

## 利用環境 
- Graphics & Touch Driver : [LovyanGFX 1.1.5](https://github.com/lovyan03/LovyanGFX)
- UI / Widgets : [LVGL 8.3.4](https://github.com/lvgl/lvgl)
- Platform : [Espressif 32: development platform for PlatformIO](https://github.com/platformio/platform-espressif32)
- Framework : arduino

## Features included 
- [x] Wi-Fi サポート
- [x] SD Card アクセス (テキスト読み書きのみ)

### Todo List
- [ ] SD Card アクセス (バイナリファイルの読み書き)
- [ ] BLE アクセス 
- [ ] 時間経過によるTFT液晶の明度OFF化、タッチによる復旧
- [ ] RS485ネットワーク
- [ ] スピーカーPin のアクセス
- [ ] Extended IO のアクセス

## 開発記録
[https://zigsow.jp/item/366021/review/377317](https://zigsow.jp/item/366021/review/377317)
