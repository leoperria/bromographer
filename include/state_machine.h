#pragma once

#include <stdint.h>

#include "events.h"

enum class MachineState : uint8_t {
    Boot,
    Ready,
    Exposing,
    Paused,
    Done,
};

enum class SelectedField : uint8_t {
    Time,
    Intensity,
};

enum class LedMode : uint8_t {
    Off,
    Solid,
    SlowBlink,
    FastBlink,
    VeryFastBlink,
};

struct PersistedSettings {
    uint16_t timeSec;
    uint8_t intensityPct;
};

struct OutputSnapshot {
    uint8_t uvDuty;  // 0..255
    LedMode ledMode;
};

struct ScreenBuffer {
    char line1[17];
    char line2[17];
};

class StateMachine {
public:
    void begin(uint32_t nowMs, const PersistedSettings& settings);
    void handleEvent(const InputEvent& event, uint32_t nowMs);
    void tick(uint32_t nowMs);

    MachineState state() const { return state_; }
    SelectedField selectedField() const { return selectedField_; }
    PersistedSettings currentSettings() const { return {setTimeSec_, intensityPct_}; }
    uint32_t remainingMs() const { return remainingMs_; }

    OutputSnapshot outputs() const;
    ScreenBuffer makeScreen(uint32_t nowMs) const;

private:
    void enterReady();
    void enterExposing(uint32_t nowMs, bool fromPaused);
    void enterPaused(uint32_t nowMs);
    void enterDone();

    void showLockedHint(uint32_t nowMs);

    static uint16_t stepTimeUp(uint16_t value);
    static uint16_t stepTimeDown(uint16_t value);

    uint8_t uvDutyFromIntensity() const;
    uint16_t displayedRemainingSec() const;

    MachineState state_ = MachineState::Boot;
    SelectedField selectedField_ = SelectedField::Time;

    uint16_t setTimeSec_ = 0;
    uint8_t intensityPct_ = 0;

    uint32_t bootStartedMs_ = 0;
    uint32_t remainingMs_ = 0;
    uint32_t runSegmentStartedMs_ = 0;
    uint32_t runSegmentStartRemainingMs_ = 0;

    uint32_t lockedHintUntilMs_ = 0;

    bool mainButtonDown_ = false;
    uint32_t mainButtonDownMs_ = 0;
    bool pressActive_ = false;
    MachineState pressStartState_ = MachineState::Boot;
    bool pressConsumed_ = false;
    bool pressCausedPause_ = false;
};

