#pragma once

#include <stdint.h>

#ifdef ARDUINO
#include <Arduino.h>
#endif

namespace Timebase {

inline uint32_t nowMs() {
#ifdef ARDUINO
    return millis();
#else
    return 0;
#endif
}

}  // namespace Timebase


