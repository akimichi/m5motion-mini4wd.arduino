m5motion-mini4wd.arduino
========================

ミニ四駆をm5motionとm5stickでコントロールするプロジェクト

## ディレクトリ構造

- `car` - m5motionでミニ四駆のモーターを制御するモジュール
- `controller` - m5stick-c と Hat Mini EncoderC でミニ四駆に信号を送るモジュール

## 設計方針

- carモジュールとcontrollerモジュールとの通信は、EspNOWを利用する

