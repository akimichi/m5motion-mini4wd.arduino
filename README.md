m5motion-mini4wd.arduino
========================

ミニ四駆をm5motionとm5stickでコントロールするプロジェクトである。

## ハードウェア

- ミニ四駆
- M5Atom Motion
- M5AtomS3
- M5Stick-c
- M5Stickc Mini Encoder HAT


## carモジュール

ミニ四駆に塔載されたm5motion を操作する。

## controllerモジュール

M5Stickに接続したロータリーエンコーダHATモジュールを利用してミニ四駆の前後方向の速度を制御する。


## 通信

carモジュールとcontrollerモジュールの通信にはESP-NOWを利用する。
玩具用途のため、通信は暗号化せずブロードキャストで送信している。

## ライセンス

MIT License。詳細は [LICENSE](LICENSE) を参照のこと。
