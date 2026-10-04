#pragma once

#include <stdint.h>

enum class InputEventType : uint8_t {
    ButtonDown,
    ButtonHold,
    ButtonUp,
    EncoderRotate,
    EncoderPress,
};

struct InputEvent {
    InputEventType type;
    int8_t value;      // EncoderRotate uses +/- detents, others use 0.
    uint32_t atMs;
};

