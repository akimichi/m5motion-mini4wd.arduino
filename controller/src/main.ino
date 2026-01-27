#include "M5Unified.h"
#include "M5HatMiniEncoderC.h"

// MiniEncoderC I2C pins
#define MiniEncoderC_SDA 0
#define MiniEncoderC_SCL 26

M5HatMiniEncoderC encoder;

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

    // Initial display
    M5.Display.setCursor(0, 20);
    M5.Display.printf("Val:%d", 0);

    M5.Display.setCursor(0, 50);
    M5.Display.printf("IncVal:%d", 0);

    M5.Display.drawLine(0, 80, 135, 80, ORANGE);

    M5.Display.setCursor(0, 90);
    M5.Display.printf("BtnVal:1");

    M5.Display.setCursor(0, 180);
    M5.Display.printf("BtnA:\n Reset Cntr");
}

void loop() {
    M5.update();

    // Read encoder value
    int32_t encoderValue = encoder.getEncoderValue();

    // Read encoder button state
    bool EncoderBtnValue = encoder.getButtonStatus();

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
      M5.Display.fillRect(0, 90, 135, 80, BLACK);
      M5.Display.setCursor(0, 90);
      M5.Display.printf("BtnVal: %d", EncoderBtnValue);
      lastEncoderBtnValue = EncoderBtnValue;
    }

    if (M5.BtnA.wasPressed()) {
      // Reset encoder value to 0 when BtnA is pressed
      encoder.resetCounter();
    }

    delay(30);
} 
