#include "M5Unified.h"
#include "M5GFX.h"
#include "M5AtomicMotion.h"
#include <esp_now.h>
#include <WiFi.h>

// ESP-NOW受信データ構造体（controllerと同じ形式）
typedef struct __attribute__((packed)) {
    uint8_t  deviceId;
    int32_t  encoderValue;
    int32_t  encoderIncValue;
    uint8_t  buttonState;
    uint32_t timestamp;
} EncoderData_t;

M5AtomicMotion AtomicMotion;

// ESP-NOW受信データ
volatile int32_t receivedEncoderValue = 0;
volatile bool dataReceived = false;

// ESP-NOW受信コールバック
void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
    if (len == sizeof(EncoderData_t)) {
        EncoderData_t data;
        memcpy(&data, incomingData, sizeof(EncoderData_t));
        receivedEncoderValue = data.encoderValue;
        dataReceived = true;
    }
}

// ESP-NOW初期化
static bool initEspNow() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    if (esp_now_init() != ESP_OK) {
        return false;
    }

    esp_now_register_recv_cb(onDataRecv);
    return true;
}

// LCD表示更新
void updateDisplay(int32_t encoderValue, int32_t speed) {
    M5.Display.fillRect(0, 0, 128, 128, BLACK);
    M5.Display.setTextColor(WHITE, BLACK);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextSize(2);

    // エンコーダー値を表示
    M5.Display.drawString(String(encoderValue), 64, 40);

    // モーター速度を表示
    M5.Display.setTextSize(1);
    M5.Display.drawString("Speed:" + String(speed), 64, 90);
}

void setup()
{
    auto cfg = M5.config();
    M5.begin(cfg);

    M5.Display.setTextColor(GREEN);
    M5.Display.setTextDatum(middle_center);
    M5.Display.setTextSize(2);
    M5.Display.drawString("Atomic Init", M5.Display.width() / 2, M5.Display.height() / 2);

    m5::board_t board = M5.getBoard();

    uint8_t sda = 0, scl = 0;

    if (board == m5::board_t::board_M5AtomLite || board == m5::board_t::board_M5AtomMatrix ||
        board == m5::board_t::board_M5AtomEcho) {
        sda = 25;
        scl = 21;
    } else if (board == m5::board_t::board_M5AtomS3 || board == m5::board_t::board_M5AtomS3R ||
               board == m5::board_t::board_M5AtomS3Lite || board == m5::board_t::board_M5AtomS3RExt ||
               board == m5::board_t::board_M5AtomS3RCam) {
        sda = 38;
        scl = 39;
    }

    while (!AtomicMotion.begin(&Wire, M5_ATOMIC_MOTION_I2C_ADDR, sda, scl, 100000)) {
        M5.Display.clear();
        M5.Display.drawString("Init Fail", M5.Display.width() / 2, M5.Display.height() / 2);
        Serial.println("Atomic Motion begin failed");
        delay(1000);
    }

    M5.Display.clear();
    M5.Display.drawString("Motion OK", M5.Display.width() / 2, M5.Display.height() / 2);

    Serial.println("Atomic Motion OK");

    // ESP-NOW初期化
    if (initEspNow()) {
        Serial.println("ESP-NOW OK");
    } else {
        Serial.println("ESP-NOW NG");
        M5.Display.clear();
        M5.Display.setTextColor(RED);
        M5.Display.drawString("ESP-NOW NG", M5.Display.width() / 2, M5.Display.height() / 2);
    }
}

void loop()
{
    if (dataReceived) {
        // encoderValueを-127〜127にクランプ
        int32_t speed = receivedEncoderValue;
        if (speed > 127) speed = 127;
        if (speed < -127) speed = -127;

        // channel 1のモーター速度を設定
        AtomicMotion.setMotorSpeed(1, speed);

        // LCD表示を更新
        updateDisplay(receivedEncoderValue, speed);

        Serial.printf("Speed: %d, Motor Speed: %d\n", receivedEncoderValue, speed);

        dataReceived = false;
    }
    delay(10);
} 

