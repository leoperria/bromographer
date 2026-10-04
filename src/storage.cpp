#include "storage.h"

#include <Arduino.h>
#include <EEPROM.h>

#include "config.h"

namespace {

constexpr uint16_t kMagic = 0x4252;  // 'BR'

struct StoredData {
    uint16_t magic;
    uint16_t timeSec;
    uint8_t intensityPct;
    uint8_t crc;
};

uint8_t checksum(const StoredData& d) {
    const uint8_t* p = reinterpret_cast<const uint8_t*>(&d);
    uint8_t sum = 0;
    for (size_t i = 0; i < sizeof(StoredData) - 1; ++i) {
        sum ^= p[i];
    }
    return sum;
}

PersistedSettings defaults() {
    return {Config::kTimeDefaultSec, Config::kIntensityDefaultPct};
}

}  // namespace

bool Storage::isIntensityValid(const uint8_t pct) const {
    if (pct < Config::kIntensityMinPct || pct > Config::kIntensityMaxPct) {
        return false;
    }
    return (pct % Config::kIntensityStepPct) == 0;
}

bool Storage::isTimeValid(const uint16_t sec) const {
    if (sec < Config::kTimeMinSec || sec > Config::kTimeMaxSec) {
        return false;
    }

    // Validate against the configured stepping grid.
    uint16_t v = Config::kTimeMinSec;
    while (v < Config::kTimeMaxSec) {
        if (v == sec) {
            return true;
        }
        if (v < 60) {
            ++v;
        } else if (v < 300) {
            v = static_cast<uint16_t>(v + 5);
        } else {
            v = static_cast<uint16_t>(v + 15);
        }
    }
    return sec == Config::kTimeMaxSec;
}

PersistedSettings Storage::loadOrDefault() {
    StoredData raw{};
    EEPROM.get(0, raw);

    PersistedSettings s = defaults();

    const bool ok = raw.magic == kMagic && raw.crc == checksum(raw) && isTimeValid(raw.timeSec) &&
                    isIntensityValid(raw.intensityPct);
    if (ok) {
        s.timeSec = raw.timeSec;
        s.intensityPct = raw.intensityPct;
    }

    lastSaved_ = s;
    hasLastSaved_ = true;
    return s;
}

void Storage::saveIfChanged(const PersistedSettings& settings) {
    if (!hasLastSaved_) {
        lastSaved_ = loadOrDefault();
    }

    if (settings.timeSec == lastSaved_.timeSec && settings.intensityPct == lastSaved_.intensityPct) {
        return;
    }

    StoredData raw{};
    raw.magic = kMagic;
    raw.timeSec = settings.timeSec;
    raw.intensityPct = settings.intensityPct;
    raw.crc = checksum(raw);

    EEPROM.put(0, raw);
    lastSaved_ = settings;
}

