# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## プロジェクト概要

M5Stick-CとMiniEncoderC（ロータリーエンコーダー）を使用したミニ四駆コントローラーのArduinoプロジェクト。PlatformIOを使用してESP32（M5Stick-C）向けにビルドする。

このプロジェクトは `m5motion-mini4wd.arduino` 全体の一部であり、別の`car`サブプロジェクト（M5Atomを使用したモーター制御側）と連携して動作する予定である。

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

- **ターゲットボード**: M5Stick-C（ESP32）
- **入力デバイス**: MiniEncoderC（I2C接続ロータリーエンコーダー）
- **I2Cピン配置**: SDA=0, SCL=26
- **ディスプレイ**: M5Stick-C内蔵LCD（135x240）

## コード構造

- `src/main.ino` - メインプログラム。エンコーダー読み取りとUI表示
- `lib/` - カスタムライブラリ用ディレクトリ（現在空）
- `include/` - ヘッダファイル用ディレクトリ（現在空）

## 依存ライブラリ

- M5Unified / M5GFX - M5Stack系デバイスの統合ライブラリ
- M5HatMiniEncoderC - MiniEncoderC制御（M5Unifiedに含まれる）
- Wire - I2C通信
- ArduinoJson - JSON処理

## 現在の機能

- MiniEncoderCの回転量から目標速度（-127〜127）をcontroller側で積算し、ESP-NOWでcar側へ送信
- 回転速度に応じたゲイン切替（クリック間隔が短いほど1クリックあたりの変化量が大きい。`GAIN_SLOW/MID/FAST` と `ROTATE_*_MS` で調整）
- エンコーダーボタンを押しながら回すと粗調整（`COARSE_MULTIPLIER` 倍）、回さずに離すと目標速度を0にして停止
- 目標速度が最大値に達したらブザーを鳴らす
- 目標速度に応じたLED色変更
- M5Stick-CのボタンAで目標速度とエンコーダーカウンターをリセット

## 通信

- car側（M5Atom + M5AtomicMotion）とはESP-NOWで通信する（実装済み）
- 玩具用途のため暗号化せず、ブロードキャストアドレス宛に `EncoderData_t` を送信する
