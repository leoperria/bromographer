#include "input.h"

#include <Arduino.h>

#include "config.h"

namespace {

int8_t decodeQuadratureStep(const uint8_t prevAB, const uint8_t currAB) {
    // 4-bit transition lookup: prevAB<<2 | currAB
    static const int8_t table[16] = {
        0, -1, +1, 0,
        +1, 0, 0, -1,
        -1, 0, 0, +1,
        0, +1, -1, 0,
    };
    return table[(prevAB << 2) | currAB];
}

}  // namespace

void Input::begin(uint32_t nowMs) {
    btnLastReading_ = digitalRead(Config::kStartStopButtonPin);
    btnStable_ = btnLastReading_;
    btnLastEdgeMs_ = nowMs;
    btnDown_ = false;
    btnDownMs_ = nowMs;
    btnHoldSent_ = false;

    encKeyLastReading_ = digitalRead(Config::kEncoderKeyPin);
    encKeyStable_ = encKeyLastReading_;
    encKeyLastEdgeMs_ = nowMs;
    encKeyDown_ = false;
    encKeyDownMs_ = nowMs;

    const uint8_t a = digitalRead(Config::kEncoderS1Pin) ? 1U : 0U;
    const uint8_t b = digitalRead(Config::kEncoderS2Pin) ? 1U : 0U;
    encPrevAB_ = static_cast<uint8_t>((a << 1) | b);
    encQuarterAccum_ = 0;
}

bool Input::push(const InputEvent& event) {
    const uint8_t next = static_cast<uint8_t>((qHead_ + 1U) % kQueueCapacity);
    if (next == qTail_) {
        return false;
    }
    queue_[qHead_] = event;
    qHead_ = next;
    return true;
}

bool Input::pop(InputEvent* event) {
    if (qHead_ == qTail_) {
        return false;
    }
    *event = queue_[qTail_];
    qTail_ = static_cast<uint8_t>((qTail_ + 1U) % kQueueCapacity);
    return true;
}

void Input::poll(uint32_t nowMs) {
    // --- Start/Stop button debounce + DOWN/HOLD/UP events (active low) ---
    const bool btnReading = digitalRead(Config::kStartStopButtonPin);
    if (btnReading != btnLastReading_) {
        btnLastReading_ = btnReading;
        btnLastEdgeMs_ = nowMs;
    }

    if ((nowMs - btnLastEdgeMs_) >= Config::kButtonDebounceMs && btnStable_ != btnReading) {
        btnStable_ = btnReading;
        if (!btnStable_) {
            btnDown_ = true;
            btnDownMs_ = nowMs;
            btnHoldSent_ = false;
            push({InputEventType::ButtonDown, 0, nowMs});
        } else {
            if (btnDown_) {
                push({InputEventType::ButtonUp, 0, nowMs});
            }
            btnDown_ = false;
            btnHoldSent_ = false;
        }
    }

    if (btnDown_ && !btnHoldSent_ && (nowMs - btnDownMs_ >= Config::kResetHoldMs)) {
        btnHoldSent_ = true;
        push({InputEventType::ButtonHold, 0, nowMs});
    }

    // --- Encoder push debounce: short press on release, long press ignored ---
    const bool encKeyReading = digitalRead(Config::kEncoderKeyPin);
    if (encKeyReading != encKeyLastReading_) {
        encKeyLastReading_ = encKeyReading;
        encKeyLastEdgeMs_ = nowMs;
    }

    if ((nowMs - encKeyLastEdgeMs_) >= Config::kButtonDebounceMs && encKeyStable_ != encKeyReading) {
        encKeyStable_ = encKeyReading;
        if (!encKeyStable_) {
            encKeyDown_ = true;
            encKeyDownMs_ = nowMs;
        } else if (encKeyDown_) {
            const uint32_t heldMs = nowMs - encKeyDownMs_;
            if (heldMs < Config::kEncoderLongPushIgnoreMs) {
                push({InputEventType::EncoderPress, 0, nowMs});
            }
            encKeyDown_ = false;
        }
    }

    // --- Encoder quadrature decode (one detent = one step) ---
    if (btnDown_) {
        // While main button is held, ignore rotation events.
        const uint8_t a = digitalRead(Config::kEncoderS1Pin) ? 1U : 0U;
        const uint8_t b = digitalRead(Config::kEncoderS2Pin) ? 1U : 0U;
        encPrevAB_ = static_cast<uint8_t>((a << 1) | b);
        encQuarterAccum_ = 0;
        return;
    }

    const uint8_t a = digitalRead(Config::kEncoderS1Pin) ? 1U : 0U;
    const uint8_t b = digitalRead(Config::kEncoderS2Pin) ? 1U : 0U;
    const uint8_t currAB = static_cast<uint8_t>((a << 1) | b);

    if (currAB != encPrevAB_) {
        int8_t step = decodeQuadratureStep(encPrevAB_, currAB);
        encPrevAB_ = currAB;

        if (Config::kReverseEncoderDirection) {
            step = static_cast<int8_t>(-step);
        }

        encQuarterAccum_ = static_cast<int8_t>(encQuarterAccum_ + step);

        while (encQuarterAccum_ >= static_cast<int8_t>(Config::kEncoderPulsesPerDetent)) {
            encQuarterAccum_ = static_cast<int8_t>(encQuarterAccum_ - Config::kEncoderPulsesPerDetent);
            push({InputEventType::EncoderRotate, +1, nowMs});
        }

        while (encQuarterAccum_ <= -static_cast<int8_t>(Config::kEncoderPulsesPerDetent)) {
            encQuarterAccum_ = static_cast<int8_t>(encQuarterAccum_ + Config::kEncoderPulsesPerDetent);
            push({InputEventType::EncoderRotate, -1, nowMs});
        }
    }
}

