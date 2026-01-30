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

// ESP-NOW送信データ構造体
typedef struct __attribute__((packed)) {
    uint8_t  deviceId;        // デバイス識別子
    int32_t  encoderValue;    // エンコーダー絶対値
    int32_t  encoderIncValue; // エンコーダー増分値
    uint8_t  buttonState;     // ボタン状態（0/1）
    uint32_t timestamp;       // millis()値
} EncoderData_t;

// ブロードキャストアドレス
static const uint8_t BROADCAST_ADDRESS[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

M5HatMiniEncoderC encoder;

// ESP-NOW送信データ
EncoderData_t sendData;
unsigned long lastSendTime = 0;

// Used to detect encoder value changes
int32_t lastEncoderValue = 0;
int32_t encoderIncValue = 0;

// Used to detect button state changes
bool lastEncoderBtnValue = 0;

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
static void sendEncoderData(int32_t encoderValue, int32_t encoderIncValue, bool buttonState) {
    sendData.deviceId = DEVICE_ID;
    sendData.encoderValue = encoderValue;
    sendData.encoderIncValue = encoderIncValue;
    sendData.buttonState = buttonState ? 1 : 0;
    sendData.timestamp = millis();

    esp_now_send(BROADCAST_ADDRESS, (uint8_t*)&sendData, sizeof(EncoderData_t));
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
    M5.Display.setCursor(0, 20);
    M5.Display.printf("Val:%d", 0);

    M5.Display.setCursor(0, 50);
    M5.Display.printf("IncVal:%d", 0);

    M5.Display.drawLine(0, 80, 135, 80, ORANGE);

    M5.Display.setCursor(0, 90);
    M5.Display.printf("BtnVal:1");

    M5.Display.setCursor(0, 150);
    M5.Display.printf("BtnA:\n Reset Cntr");
}

void loop() {
    M5.update();

    // Read encoder value
    int32_t encoderValue = encoder.getEncoderValue();

    // Read encoder button state
    bool EncoderBtnValue = encoder.getButtonStatus();

    // 値変更検出
    bool valueChanged = (encoderValue != lastEncoderValue) ||
                        (EncoderBtnValue != lastEncoderBtnValue);

    // Only read increment value when encoder value changes
    if (encoderValue != lastEncoderValue) {
      encoderIncValue = encoder.getIncrementValue();

      // Update encoder value display
      M5.Display.fillRect(0, 20, 135, 50, BLACK);
      M5.Display.setTextColor(WHITE, BLACK);

      M5.Display.setCursor(0, 20);
      M5.Display.printf("Val: %d", encoderValue);

      M5.Display.setCursor(0, 50);
      M5.Display.printf("IncVal: %d", encoderIncValue);

      M5.Display.drawLine(0, 80, 135, 80, ORANGE);

      // Set LED color based on encoder value
      uint8_t r = abs(encoderValue * 5) % 256;
      uint8_t g = abs(encoderValue * 3) % 256;
      uint8_t b = abs(encoderValue * 7) % 256;
      uint32_t rgb888 = (r << 16) | (g << 8) | b;
      encoder.setLEDColor(rgb888);

      lastEncoderValue = encoderValue;
    }

    // Update display only when button state changes
    if (EncoderBtnValue != lastEncoderBtnValue) {
      M5.Display.fillRect(0, 90, 135, 50, BLACK);
      M5.Display.setCursor(0, 90);
      M5.Display.printf("BtnVal: %d", EncoderBtnValue);
      lastEncoderBtnValue = EncoderBtnValue;
    }

    // ESP-NOW送信（値変更時または一定間隔）
    unsigned long currentTime = millis();
    if (valueChanged || (currentTime - lastSendTime >= SEND_INTERVAL_MS)) {
      sendEncoderData(encoderValue, encoderIncValue, EncoderBtnValue);
      lastSendTime = currentTime;
    }

    if (M5.BtnA.wasPressed()) {
      // Reset encoder value to 0 when BtnA is pressed
      encoder.resetCounter();
    }

    delay(30);
} 
