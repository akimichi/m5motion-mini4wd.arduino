# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## プロジェクト概要

M5Motion（M5AtomicMotion）を利用してミニ四駆のDCモーターを制御するArduinoプロジェクト。PlatformIOを使用してESP32（M5Stack系ボード）向けにビルドする。

## ビルドコマンド

```bash
# ビルド
pio run

# ビルドしてアップロード
pio run -t upload

# シリアルモニター
pio device monitor

# ビルド＆アップロード＆モニター
pio run -t upload && pio device monitor
```

## ハードウェア構成

- **ターゲットボード**: M5Stick-C / M5Atom系（ESP32）
- **モータードライバー**: M5AtomicMotion（I2C接続）
- **I2Cピン配置**:
  - M5Atom Lite/Matrix/Echo: SDA=25, SCL=21
  - M5AtomS3系: SDA=38, SCL=39

## コード構造

- `src/main.ino` - メインプログラム。モーター制御のセットアップとループ処理
- `lib/` - カスタムライブラリ用ディレクトリ（現在空）
- `include/` - ヘッダファイル用ディレクトリ（現在空）

## 依存ライブラリ

- M5Unified / M5GFX - M5Stack系デバイスの統合ライブラリ
- M5Atom - M5Atom向けライブラリ
- FastLED - LED制御（lib_depsに含まれる）
- ArduinoJson - JSON処理
