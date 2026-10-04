#pragma once

#include "state_machine.h"

class Storage {
public:
    PersistedSettings loadOrDefault();
    void saveIfChanged(const PersistedSettings& settings);

private:
    bool isTimeValid(uint16_t sec) const;
    bool isIntensityValid(uint8_t pct) const;

    PersistedSettings lastSaved_{};
    bool hasLastSaved_ = false;
};

