#pragma once

#include <stdint.h>

#include "events.h"

class Input {
public:
    void begin(uint32_t nowMs);
    void poll(uint32_t nowMs);
    bool pop(InputEvent* event);

private:
    bool push(const InputEvent& event);

    static constexpr uint8_t kQueueCapacity = 16;
    InputEvent queue_[kQueueCapacity]{};
    uint8_t qHead_ = 0;
    uint8_t qTail_ = 0;

    // Main button (D6)
    bool btnLastReading_ = true;
    bool btnStable_ = true;
    uint32_t btnLastEdgeMs_ = 0;
    bool btnDown_ = false;
    uint32_t btnDownMs_ = 0;
    bool btnHoldSent_ = false;

    // Encoder push (D9)
    bool encKeyLastReading_ = true;
    bool encKeyStable_ = true;
    uint32_t encKeyLastEdgeMs_ = 0;
    bool encKeyDown_ = false;
    uint32_t encKeyDownMs_ = 0;

    // Encoder quadrature (D7/D8)
    uint8_t encPrevAB_ = 0;
    int8_t encQuarterAccum_ = 0;
};

