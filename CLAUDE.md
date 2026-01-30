# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## プロジェクト概要

ミニ四駆をm5motionとm5stickでコントロールするプロジェクト


## ディレクトリ構造

- `car` - m5motionでミニ四駆のモーターを制御するモジュール
- `controller` - m5stick-c と Hat Mini EncoderC でミニ四駆に信号を送るモジュール

## 設計方針

- carモジュールとcontrollerモジュールとの通信は、EspNOWを利用する


