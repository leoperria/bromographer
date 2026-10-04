#include "outputs.h"

#include <Arduino.h>
#include <avr/io.h>

#include "config.h"

namespace {

void configureFastPwmOnD5() {
    // ATmega32U4 D5 = OC3A. Configure Timer3 for 8-bit fast PWM, prescaler=1.
    // Frequency: 16MHz / 256 = 62.5kHz (non-inverting on OC3A).
    pinMode(Config::kUvPwmPin, OUTPUT);
    TCCR3A = _BV(COM3A1) | _BV(WGM30);
    TCCR3B = _BV(WGM32) | _BV(CS30);
    OCR3A = 0;
}

void setUvDuty(uint8_t duty) {
    OCR3A = duty;
}

uint8_t pctToDuty(uint8_t pct) {
    if (pct >= 100) {
        return 255;
    }
    return static_cast<uint8_t>((static_cast<uint16_t>(pct) * 255U) / 100U);
}

void setBuzzerEnabled(bool on) {
    if (!on) {
        analogWrite(Config::kBuzzerPin, 0);
        return;
    }

    uint8_t duty = pctToDuty(Config::kBuzzerPwmDutyPct);
    if (duty == 0) {
        duty = 1;
    }
    analogWrite(Config::kBuzzerPin, duty);
}

}  // namespace

void Outputs::forceUvOffEarly() {
    // Must be called at the top of setup for startup safety.
    configureFastPwmOnD5();
    setUvDuty(0);
}

void Outputs::begin(uint32_t nowMs) {
    pinMode(Config::kStatusLedPin, OUTPUT);
    digitalWrite(Config::kStatusLedPin, LOW);
    pinMode(Config::kBuzzerPin, OUTPUT);
    setBuzzerEnabled(false);

    configureFastPwmOnD5();
    setUvDuty(0);

    uvDuty_ = 0;
    ledMode_ = LedMode::Off;
    ledOn_ = false;
    lastLedEdgeMs_ = nowMs;
    buzzerOn_ = false;
    buzzerUntilMs_ = 0;
    buzzerMode_ = BuzzerMode::Off;
}

void Outputs::apply(const OutputSnapshot& snapshot, uint32_t nowMs) {
    if (snapshot.uvDuty != uvDuty_) {
        uvDuty_ = snapshot.uvDuty;
        setUvDuty(uvDuty_);
    }

    if (buzzerMode_ != BuzzerMode::Off && static_cast<int32_t>(nowMs - buzzerUntilMs_) >= 0) {
        if (buzzerMode_ == BuzzerMode::SinglePulse) {
            stopBuzz();
        } else {
            buzzerOn_ = !buzzerOn_;
            setBuzzerEnabled(buzzerOn_);
            buzzerUntilMs_ = nowMs + (buzzerOn_ ? Config::kDoneBuzzOnMs : Config::kDoneBuzzOffMs);
        }
    }

    applyLedMode(snapshot.ledMode, nowMs);
}

void Outputs::triggerStartBuzz(uint32_t nowMs) {
    buzzerMode_ = BuzzerMode::SinglePulse;
    buzzerOn_ = true;
    buzzerUntilMs_ = nowMs + Config::kStartBuzzMs;
    setBuzzerEnabled(true);
}

void Outputs::startDoneBuzz(uint32_t nowMs) {
    buzzerMode_ = BuzzerMode::Repeating;
    buzzerOn_ = true;
    buzzerUntilMs_ = nowMs + Config::kDoneBuzzOnMs;
    setBuzzerEnabled(true);
}

void Outputs::stopBuzz() {
    buzzerMode_ = BuzzerMode::Off;
    buzzerOn_ = false;
    buzzerUntilMs_ = 0;
    setBuzzerEnabled(false);
}

void Outputs::applyLedMode(LedMode mode, uint32_t nowMs) {
    if (mode != ledMode_) {
        ledMode_ = mode;
        lastLedEdgeMs_ = nowMs;

        if (ledMode_ == LedMode::Off) {
            ledOn_ = false;
            digitalWrite(Config::kStatusLedPin, LOW);
            return;
        }
        if (ledMode_ == LedMode::Solid) {
            ledOn_ = true;
            digitalWrite(Config::kStatusLedPin, HIGH);
            return;
        }

        // Blink mode enters with a full ON phase.
        ledOn_ = true;
        digitalWrite(Config::kStatusLedPin, HIGH);
        return;
    }

    if (ledMode_ == LedMode::Off || ledMode_ == LedMode::Solid) {
        return;
    }

    uint16_t onMs = Config::kLedFastBlinkOnMs;
    uint16_t offMs = Config::kLedFastBlinkOffMs;
    if (ledMode_ == LedMode::SlowBlink) {
        onMs = Config::kLedSlowBlinkOnMs;
        offMs = Config::kLedSlowBlinkOffMs;
    } else if (ledMode_ == LedMode::VeryFastBlink) {
        onMs = Config::kLedVeryFastBlinkOnMs;
        offMs = Config::kLedVeryFastBlinkOffMs;
    }
    const uint16_t phaseMs = ledOn_ ? onMs : offMs;

    if ((nowMs - lastLedEdgeMs_) >= phaseMs) {
        lastLedEdgeMs_ = nowMs;
        ledOn_ = !ledOn_;
        digitalWrite(Config::kStatusLedPin, ledOn_ ? HIGH : LOW);
    }
}

