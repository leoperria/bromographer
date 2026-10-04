#pragma once

#include <stdint.h>

namespace Config {

// ---- Fixed hardware mapping (validated on real board) ----
constexpr uint8_t kEncoderS1Pin = 7;
constexpr uint8_t kEncoderS2Pin = 8;
constexpr uint8_t kEncoderKeyPin = 9;
constexpr uint8_t kUvPwmPin = 5;
constexpr uint8_t kStatusLedPin = 4;
constexpr uint8_t kBuzzerPin = 10;
constexpr uint8_t kStartStopButtonPin = 6;

constexpr uint8_t kLcdI2cAddress = 0x27;
constexpr uint8_t kLcdCols = 16;
constexpr uint8_t kLcdRows = 2;

constexpr char kFirmwareRevision[] = "1";

// ---- Behavior constants from the approved plan ----
constexpr uint16_t kTimeMinSec = 5;
constexpr uint16_t kTimeMaxSec = 1800;
constexpr uint16_t kTimeDefaultSec = 150;

constexpr uint8_t kIntensityMinPct = 10;
constexpr uint8_t kIntensityMaxPct = 100;
constexpr uint8_t kIntensityStepPct = 5;
constexpr uint8_t kIntensityDefaultPct = 100;

constexpr uint16_t kResetHoldMs = 2000;
constexpr uint16_t kResetOverlayDelayMs = 250;
constexpr uint16_t kSettingsLockedHintMs = 1500;
constexpr uint16_t kEncoderLongPushIgnoreMs = 1000;
constexpr uint16_t kBootSplashMs = 2000;
constexpr uint16_t kStartBuzzMs = 80;
constexpr uint16_t kDoneBuzzOnMs = 1000;
constexpr uint16_t kDoneBuzzOffMs = 3000;
constexpr uint8_t kBuzzerPwmDutyPct = 20;

constexpr uint16_t kLedSlowBlinkOnMs = 800;
constexpr uint16_t kLedSlowBlinkOffMs = 800;
constexpr uint16_t kLedFastBlinkOnMs = 200;
constexpr uint16_t kLedFastBlinkOffMs = 200;
constexpr uint16_t kLedVeryFastBlinkOnMs = 100;
constexpr uint16_t kLedVeryFastBlinkOffMs = 100;

constexpr uint16_t kButtonDebounceMs = 25;

// One detent = this many quadrature pulses for this encoder.
constexpr uint8_t kEncoderPulsesPerDetent = 4;
constexpr bool kReverseEncoderDirection = false;

// Open decision defaults (selected for now):
// - Linear PWM mapping
// - Active-low switches with INPUT_PULLUP
// - I2C LCD

}  // namespace Config


