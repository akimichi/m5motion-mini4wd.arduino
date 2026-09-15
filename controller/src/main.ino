#include "M5Unified.h"
#include "M5HatMiniEncoderC.h"
#include <esp_now.h>
#include <WiFi.h>

// MiniEncoderC I2C pins
#define MiniEncoderC_SDA 0
#define MiniEncoderC_SCL 26

// ESP-NOW settings
#define DEVICE_ID 0x01
#define SEND_INTERVAL_MS 50

// ループ周期（回転速度の分解能を確保するため短めにする）
#define LOOP_DELAY_MS 10

// 目標速度の範囲
#define SPEED_MAX 127

// 回転速度に応じたゲイン（案1）
// クリック間隔がこの時間より短ければ、そのゲインを適用する
#define ROTATE_FAST_MS 50     // これより速い → GAIN_FAST
#define ROTATE_MID_MS  150    // これより速い → GAIN_MID、それ以外 → GAIN_SLOW
#define GAIN_SLOW 1
#define GAIN_MID  3
#define GAIN_FAST 8

// エンコーダーボタン併用（案4）
// 押しながら回すと粗調整倍率を掛ける。回さずに離すと停止
#define COARSE_MULTIPLIER 5

// ブザー関連（目標速度が最大に達したら鳴らす）
#define BUZZER_THRESHOLD SPEED_MAX
#define BUZZER_FREQ 2000      // ブザー周波数（Hz）
#define BUZZER_ON_MS 100      // ブザーON時間（ms）
#define BUZZER_OFF_MS 100     // ブザーOFF時間（ms）

// ESP-NOW送信データ構造体
typedef struct __attribute__((packed)) {
    uint8_t  deviceId;        // デバイス識別子
    int32_t  encoderValue;    // 目標速度（-127〜127）
    int32_t  encoderIncValue; // このループで目標速度に加えた量
    uint8_t  buttonState;     // ボタン状態（0:押下 / 1:開放）
    uint32_t timestamp;       // millis()値
} EncoderData_t;

// ブロードキャストアドレス
static const uint8_t BROADCAST_ADDRESS[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

M5HatMiniEncoderC encoder;

// ESP-NOW送信データ
EncoderData_t sendData;
unsigned long lastSendTime = 0;

// エンコーダー生値の前回値（差分検出用）
int32_t lastEncoderValue = 0;
// 最後にエンコーダーが動いた時刻（回転速度の判定用）
unsigned long lastRotateTime = 0;

// 目標速度（controller側で積算する仮想値）
int32_t targetSpeed = 0;
int32_t lastDisplayedSpeed = 0;
int32_t lastDisplayedGain = 0;

// エンコーダーボタン状態管理
bool lastEncoderBtnValue = 1;   // 1:開放 / 0:押下
bool rotatedWhileHeld = false;  // ボタン押下中に回転したか

// ブザー状態管理
unsigned long lastBuzzerToggle = 0;
bool buzzerState = false;

// Wait until MiniEncoderC is ready
static void waitMiniEncoderCReady() {
    while (!encoder.begin(&Wire, MiniEncoderC_ADDR, MiniEncoderC_SDA, MiniEncoderC_SCL, 100000UL)) {
      delay(100);
    }
}

// ESP-NOW初期化
static bool initEspNow() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    if (esp_now_init() != ESP_OK) {
        return false;
    }

    // ブロードキャスト用ピア登録
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, BROADCAST_ADDRESS, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        return false;
    }

    return true;
}

// ESP-NOWデータ送信
static void sendEncoderData(int32_t speed, int32_t appliedDelta, bool buttonState) {
    sendData.deviceId = DEVICE_ID;
    sendData.encoderValue = constrain(speed, -SPEED_MAX, SPEED_MAX);
    sendData.encoderIncValue = appliedDelta;
    sendData.buttonState = buttonState ? 1 : 0;
    sendData.timestamp = millis();

    esp_now_send(BROADCAST_ADDRESS, (uint8_t*)&sendData, sizeof(EncoderData_t));
}

// クリック間隔から回転速度ゲインを決める
static int32_t rotationGain(unsigned long interval, int32_t delta) {
    // 1ループ内で2クリック以上進んでいれば十分速いとみなす
    if (abs(delta) >= 2 || interval < ROTATE_FAST_MS) return GAIN_FAST;
    if (interval < ROTATE_MID_MS) return GAIN_MID;
    return GAIN_SLOW;
}

// 目標速度・ゲイン表示更新
static void updateSpeedDisplay(int32_t speed, int32_t gain) {
    M5.Display.fillRect(0, 20, 135, 50, BLACK);
    M5.Display.setTextColor(WHITE, BLACK);

    M5.Display.setCursor(0, 20);
    M5.Display.printf("Spd: %d", speed);

    M5.Display.setCursor(0, 50);
    M5.Display.printf("Gain: x%d", gain);

    M5.Display.drawLine(0, 80, 135, 80, ORANGE);
}

// ボタン状態表示更新
static void updateButtonDisplay(bool btnValue) {
    M5.Display.fillRect(0, 90, 135, 30, BLACK);
    M5.Display.setCursor(0, 90);
    M5.Display.printf("Btn: %s", btnValue ? "-" : "HOLD");
}

// 目標速度をリセットし、エンコーダーの生値も0に揃える
static void resetSpeed() {
    targetSpeed = 0;
    encoder.resetCounter();
    delay(20);
    lastEncoderValue = encoder.getEncoderValue();
}

void setup() {
    M5.begin();
    M5.Display.setRotation(0);
    M5.Display.setFont(&fonts::FreeMonoBold9pt7b);
    M5.Display.fillScreen(BLACK);

    // Initialize MiniEncoderC
    waitMiniEncoderCReady();

    // Reset encoder value to 0
    encoder.setEncoderValue(0);
    delay(100);
    lastEncoderValue = encoder.getEncoderValue();

    // Initialize ESP-NOW
    if (initEspNow()) {
        M5.Display.setCursor(0, 220);
        M5.Display.setTextColor(GREEN, BLACK);
        M5.Display.printf("ESP-NOW OK");
    } else {
        M5.Display.setCursor(0, 220);
        M5.Display.setTextColor(RED, BLACK);
        M5.Display.printf("ESP-NOW NG");
    }
    M5.Display.setTextColor(WHITE, BLACK);

    // Initial display
    updateSpeedDisplay(0, GAIN_SLOW);
    updateButtonDisplay(1);

    M5.Display.setCursor(0, 130);
    M5.Display.printf("Click:Stop\nHold :x%d", COARSE_MULTIPLIER);

    M5.Display.setCursor(0, 180);
    M5.Display.printf("BtnA:Reset");
}

void loop() {
    M5.update();
    unsigned long now = millis();

    // Read encoder value and button state
    int32_t encoderValue = encoder.getEncoderValue();
    bool encoderBtnValue = encoder.getButtonStatus();   // 1:開放 / 0:押下
    bool btnHeld = (encoderBtnValue == 0);

    // ボタン押下エッジ: 押下中の回転フラグをクリア
    if (btnHeld && lastEncoderBtnValue == 1) {
        rotatedWhileHeld = false;
    }

    // 回転量に応じて目標速度を更新
    int32_t delta = encoderValue - lastEncoderValue;
    int32_t appliedDelta = 0;
    int32_t gain = lastDisplayedGain;
    if (delta != 0) {
        gain = rotationGain(now - lastRotateTime, delta);
        if (btnHeld) {
            gain *= COARSE_MULTIPLIER;
            rotatedWhileHeld = true;
        }
        appliedDelta = delta * gain;
        targetSpeed = constrain(targetSpeed + appliedDelta, -SPEED_MAX, SPEED_MAX);

        lastEncoderValue = encoderValue;
        lastRotateTime = now;
    }

    // ボタン開放エッジ: 押下中に回転していなければ単押しとみなして停止
    if (!btnHeld && lastEncoderBtnValue == 0 && !rotatedWhileHeld) {
        targetSpeed = 0;
    }

    bool valueChanged = (targetSpeed != lastDisplayedSpeed) ||
                        (encoderBtnValue != lastEncoderBtnValue);

    // 表示・LED更新
    if (targetSpeed != lastDisplayedSpeed || gain != lastDisplayedGain) {
        updateSpeedDisplay(targetSpeed, gain);

        // Set LED color based on target speed
        uint8_t r = abs(targetSpeed * 5) % 256;
        uint8_t g = abs(targetSpeed * 3) % 256;
        uint8_t b = abs(targetSpeed * 7) % 256;
        uint32_t rgb888 = (r << 16) | (g << 8) | b;
        encoder.setLEDColor(rgb888);

        lastDisplayedSpeed = targetSpeed;
        lastDisplayedGain = gain;
    }

    if (encoderBtnValue != lastEncoderBtnValue) {
        updateButtonDisplay(encoderBtnValue);
        lastEncoderBtnValue = encoderBtnValue;
    }

    // ESP-NOW送信（値変更時または一定間隔）
    if (valueChanged || (now - lastSendTime >= SEND_INTERVAL_MS)) {
        sendEncoderData(targetSpeed, appliedDelta, encoderBtnValue);
        lastSendTime = now;
    }

    if (M5.BtnA.wasPressed()) {
        resetSpeed();
    }

    // ブザー制御（目標速度が最大に達した場合）
    if (targetSpeed >= BUZZER_THRESHOLD || targetSpeed <= -BUZZER_THRESHOLD) {
        if (now - lastBuzzerToggle >= (buzzerState ? BUZZER_ON_MS : BUZZER_OFF_MS)) {
            buzzerState = !buzzerState;
            if (buzzerState) {
                M5.Speaker.tone(BUZZER_FREQ, BUZZER_ON_MS);
            }
            lastBuzzerToggle = now;
        }
    } else {
        // 最大値未満に戻ったらブザーを停止
        if (buzzerState) {
            M5.Speaker.stop();
            buzzerState = false;
        }
    }

    delay(LOOP_DELAY_MS);
}
