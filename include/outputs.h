#pragma once

#include <stdint.h>

#include "state_machine.h"

class Outputs {
public:
    static void forceUvOffEarly();

    void begin(uint32_t nowMs);
    void apply(const OutputSnapshot& snapshot, uint32_t nowMs);
    void triggerStartBuzz(uint32_t nowMs);
    void startDoneBuzz(uint32_t nowMs);
    void stopBuzz();

private:
    enum class BuzzerMode : uint8_t {
        Off,
        SinglePulse,
        Repeating,
    };

    void applyLedMode(LedMode mode, uint32_t nowMs);

    LedMode ledMode_ = LedMode::Off;
    bool ledOn_ = false;
    uint32_t lastLedEdgeMs_ = 0;
    uint8_t uvDuty_ = 0;
    bool buzzerOn_ = false;
    uint32_t buzzerUntilMs_ = 0;
    BuzzerMode buzzerMode_ = BuzzerMode::Off;
};

