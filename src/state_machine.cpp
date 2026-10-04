#include "state_machine.h"

#include <stdio.h>
#include <string.h>

#include "config.h"

namespace {

uint8_t mapIntensityToDuty(const uint8_t pct) {
    return static_cast<uint8_t>((static_cast<uint16_t>(pct) * 255U) / 100U);
}

void writeTimeAtRight(char* line, const uint16_t sec) {
    const uint16_t mm = sec / 60;
    const uint16_t ss = sec % 60;
    char timeText[6];
    snprintf(timeText, sizeof(timeText), "%02u:%02u", mm, ss);
    memcpy(&line[11], timeText, 5);
}

void writeRightAlignedIntensity(char* line, const uint8_t intensityPct) {
    char p[6];
    snprintf(p, sizeof(p), "%u%%", intensityPct);
    const size_t len = strlen(p);
    const int start = 16 - static_cast<int>(len);
    memcpy(&line[start], p, len);
}

}  // namespace

void StateMachine::begin(uint32_t nowMs, const PersistedSettings& settings) {
    bootStartedMs_ = nowMs;
    state_ = MachineState::Boot;
    selectedField_ = SelectedField::Time;

    setTimeSec_ = settings.timeSec;
    intensityPct_ = settings.intensityPct;
    remainingMs_ = static_cast<uint32_t>(setTimeSec_) * 1000UL;

    lockedHintUntilMs_ = 0;

    mainButtonDown_ = false;
    mainButtonDownMs_ = 0;
    pressActive_ = false;
    pressStartState_ = MachineState::Boot;
    pressConsumed_ = false;
    pressCausedPause_ = false;
}

void StateMachine::handleEvent(const InputEvent& event, uint32_t nowMs) {
    switch (event.type) {
        case InputEventType::ButtonDown: {
            mainButtonDown_ = true;
            mainButtonDownMs_ = nowMs;
            pressActive_ = true;
            pressStartState_ = state_;
            pressConsumed_ = false;
            pressCausedPause_ = false;

            if (state_ == MachineState::Exposing) {
                enterPaused(nowMs);
                pressCausedPause_ = true;
            }
            break;
        }

        case InputEventType::ButtonHold: {
            if (state_ == MachineState::Paused && pressActive_ && !pressConsumed_) {
                enterReady();
                pressConsumed_ = true;
            }
            break;
        }

        case InputEventType::ButtonUp: {
            mainButtonDown_ = false;
            if (!pressActive_) {
                break;
            }

            if (!pressConsumed_) {
                if (state_ == MachineState::Ready && pressStartState_ == MachineState::Ready) {
                    enterExposing(nowMs, false);
                } else if (state_ == MachineState::Paused) {
                    if (pressStartState_ == MachineState::Paused) {
                        enterExposing(nowMs, true);
                    } else if (pressCausedPause_) {
                        // Release of the same press that paused must not auto-resume.
                    }
                } else if (state_ == MachineState::Done && pressStartState_ == MachineState::Done) {
                    enterReady();
                }
            }

            pressActive_ = false;
            pressConsumed_ = false;
            pressCausedPause_ = false;
            break;
        }

        case InputEventType::EncoderRotate: {
            if (mainButtonDown_) {
                break;
            }

            if (state_ == MachineState::Ready) {
                int8_t detents = event.value;
                while (detents > 0) {
                    if (selectedField_ == SelectedField::Time) {
                        setTimeSec_ = stepTimeUp(setTimeSec_);
                    } else {
                        intensityPct_ = static_cast<uint8_t>(
                            (intensityPct_ + Config::kIntensityStepPct > Config::kIntensityMaxPct)
                                ? Config::kIntensityMaxPct
                                : intensityPct_ + Config::kIntensityStepPct);
                    }
                    --detents;
                }
                while (detents < 0) {
                    if (selectedField_ == SelectedField::Time) {
                        setTimeSec_ = stepTimeDown(setTimeSec_);
                    } else {
                        intensityPct_ = static_cast<uint8_t>(
                            (intensityPct_ < Config::kIntensityMinPct + Config::kIntensityStepPct)
                                ? Config::kIntensityMinPct
                                : intensityPct_ - Config::kIntensityStepPct);
                    }
                    ++detents;
                }
                remainingMs_ = static_cast<uint32_t>(setTimeSec_) * 1000UL;
            } else if (state_ == MachineState::Exposing || state_ == MachineState::Paused) {
                showLockedHint(nowMs);
            }
            break;
        }

        case InputEventType::EncoderPress: {
            if (mainButtonDown_) {
                break;
            }

            if (state_ == MachineState::Ready) {
                selectedField_ =
                    (selectedField_ == SelectedField::Time) ? SelectedField::Intensity : SelectedField::Time;
            } else if (state_ == MachineState::Exposing || state_ == MachineState::Paused) {
                showLockedHint(nowMs);
            } else if (state_ == MachineState::Done) {
                enterReady();
            }
            break;
        }
    }
}

void StateMachine::tick(uint32_t nowMs) {
    if (state_ == MachineState::Boot) {
        if (nowMs - bootStartedMs_ >= Config::kBootSplashMs) {
            enterReady();
        }
        return;
    }

    if (state_ == MachineState::Exposing) {
        const uint32_t elapsed = nowMs - runSegmentStartedMs_;
        if (elapsed >= runSegmentStartRemainingMs_) {
            remainingMs_ = 0;
            enterDone();
        } else {
            remainingMs_ = runSegmentStartRemainingMs_ - elapsed;
        }
    }
}

OutputSnapshot StateMachine::outputs() const {
    OutputSnapshot out{};

    if (state_ == MachineState::Exposing) {
        out.uvDuty = uvDutyFromIntensity();
        out.ledMode = LedMode::SlowBlink;
    } else if (state_ == MachineState::Paused) {
        out.uvDuty = 0;
        out.ledMode = LedMode::VeryFastBlink;
    } else if (state_ == MachineState::Done) {
        out.uvDuty = 0;
        out.ledMode = LedMode::FastBlink;
    } else {
        out.uvDuty = 0;
        out.ledMode = LedMode::Off;
    }

    return out;
}

ScreenBuffer StateMachine::makeScreen(uint32_t nowMs) const {
    ScreenBuffer s{};
    memset(s.line1, ' ', 16);
    memset(s.line2, ' ', 16);
    s.line1[16] = '\0';
    s.line2[16] = '\0';

    const bool showLocked =
        (state_ == MachineState::Exposing || state_ == MachineState::Paused) && (nowMs < lockedHintUntilMs_);

    const bool showResetOverlay =
        state_ == MachineState::Paused && mainButtonDown_ && !pressConsumed_ &&
        (nowMs - mainButtonDownMs_ >= Config::kResetOverlayDelayMs);

    if (state_ == MachineState::Done) {
        memcpy(s.line1, "Done       00:00", 16);
        memcpy(s.line2, "Press to confirm", 16);
        return s;
    }

    if (showResetOverlay) {
        memcpy(s.line1, "Hold to reset...", 16);

        const uint32_t held = nowMs - mainButtonDownMs_;
        const uint32_t capped = (held > Config::kResetHoldMs) ? Config::kResetHoldMs : held;
        const uint16_t totalUnits = 16U * 5U;
        const uint16_t units = static_cast<uint16_t>((capped * totalUnits) / Config::kResetHoldMs);
        const uint8_t fullCells = static_cast<uint8_t>(units / 5U);
        const uint8_t partial = static_cast<uint8_t>(units % 5U);

        for (uint8_t i = 0; i < fullCells && i < 16; ++i) {
            s.line2[i] = 4;  // custom char index 4 = full bar cell
        }
        if (fullCells < 16 && partial > 0) {
            s.line2[fullCells] = static_cast<char>(partial - 1U);  // indices 0..3
        }
        return s;
    }

    if (state_ == MachineState::Boot) {
        memcpy(s.line1, "UV exposure     ", 16);

        char revLine[17];
        memset(revLine, ' ', 16);
        revLine[16] = '\0';
        snprintf(revLine, sizeof(revLine), "FW rev %s", Config::kFirmwareRevision);
        for (uint8_t i = 0; i < 16; ++i) {
            if (revLine[i] == '\0') {
                revLine[i] = ' ';
            }
        }
        memcpy(s.line2, revLine, 16);
        return s;
    }

    if (state_ == MachineState::Ready) {
        if (selectedField_ == SelectedField::Time) {
            s.line1[0] = 5;  // custom char index 5 = selector arrow
        }
        memcpy(&s.line1[1], "Time      ", 10);
        writeTimeAtRight(s.line1, setTimeSec_);

        if (selectedField_ == SelectedField::Intensity) {
            s.line2[0] = 5;
        }
        memcpy(&s.line2[1], "Intensity", 9);
        writeRightAlignedIntensity(s.line2, intensityPct_);
        return s;
    }

    if (state_ == MachineState::Exposing || state_ == MachineState::Paused) {
        if (state_ == MachineState::Exposing) {
            memcpy(s.line1, "Exposing        ", 16);
        } else {
            memcpy(s.line1, "Paused          ", 16);
        }
        writeTimeAtRight(s.line1, displayedRemainingSec());

        if (showLocked) {
            memcpy(s.line2, "Settings locked ", 16);
            return s;
        }

        if (state_ == MachineState::Paused) {
            memcpy(s.line2, "Btn:go  Hold:rst", 16);
            return s;
        }

        // Exposing line 2: progress bar + space + intensity.
        char intensityText[6];
        snprintf(intensityText, sizeof(intensityText), "%u%%", intensityPct_);
        const uint8_t intensityLen = static_cast<uint8_t>(strlen(intensityText));
        const uint8_t barCells = static_cast<uint8_t>(16 - 1 - intensityLen);

        const uint32_t totalMs = static_cast<uint32_t>(setTimeSec_) * 1000UL;
        const uint32_t elapsedMs = (totalMs >= remainingMs_) ? (totalMs - remainingMs_) : 0;
        const uint16_t totalUnits = static_cast<uint16_t>(barCells * 5U);
        const uint16_t units = (totalMs == 0) ? 0 : static_cast<uint16_t>((elapsedMs * totalUnits) / totalMs);

        const uint8_t fullCells = static_cast<uint8_t>(units / 5U);
        const uint8_t partial = static_cast<uint8_t>(units % 5U);

        for (uint8_t i = 0; i < fullCells && i < barCells; ++i) {
            s.line2[i] = 4;
        }
        if (fullCells < barCells && partial > 0) {
            s.line2[fullCells] = static_cast<char>(partial - 1U);
        }

        s.line2[barCells] = ' ';
        memcpy(&s.line2[barCells + 1], intensityText, intensityLen);
    }

    return s;
}

void StateMachine::enterReady() {
    state_ = MachineState::Ready;
    remainingMs_ = static_cast<uint32_t>(setTimeSec_) * 1000UL;
    lockedHintUntilMs_ = 0;
}

void StateMachine::enterExposing(uint32_t nowMs, bool fromPaused) {
    state_ = MachineState::Exposing;
    if (!fromPaused) {
        remainingMs_ = static_cast<uint32_t>(setTimeSec_) * 1000UL;
    }
    runSegmentStartRemainingMs_ = remainingMs_;
    runSegmentStartedMs_ = nowMs;
    lockedHintUntilMs_ = 0;
}

void StateMachine::enterPaused(uint32_t nowMs) {
    if (state_ == MachineState::Exposing) {
        const uint32_t elapsed = nowMs - runSegmentStartedMs_;
        remainingMs_ = (elapsed >= runSegmentStartRemainingMs_) ? 0 : (runSegmentStartRemainingMs_ - elapsed);
    }
    state_ = MachineState::Paused;
}

void StateMachine::enterDone() {
    state_ = MachineState::Done;
    remainingMs_ = 0;
    lockedHintUntilMs_ = 0;
}

void StateMachine::showLockedHint(uint32_t nowMs) {
    lockedHintUntilMs_ = nowMs + Config::kSettingsLockedHintMs;
}

uint16_t StateMachine::stepTimeUp(uint16_t value) {
    if (value >= Config::kTimeMaxSec) {
        return Config::kTimeMaxSec;
    }
    if (value < 60) {
        return static_cast<uint16_t>((value + 1 > Config::kTimeMaxSec) ? Config::kTimeMaxSec : value + 1);
    }
    if (value < 300) {
        return static_cast<uint16_t>((value + 5 > Config::kTimeMaxSec) ? Config::kTimeMaxSec : value + 5);
    }
    return static_cast<uint16_t>((value + 15 > Config::kTimeMaxSec) ? Config::kTimeMaxSec : value + 15);
}

uint16_t StateMachine::stepTimeDown(uint16_t value) {
    if (value <= Config::kTimeMinSec) {
        return Config::kTimeMinSec;
    }
    if (value <= 60) {
        return static_cast<uint16_t>((value <= Config::kTimeMinSec) ? Config::kTimeMinSec : value - 1);
    }
    if (value <= 300) {
        return static_cast<uint16_t>((value < 5 + Config::kTimeMinSec) ? Config::kTimeMinSec : value - 5);
    }
    return static_cast<uint16_t>((value < 15 + Config::kTimeMinSec) ? Config::kTimeMinSec : value - 15);
}

uint8_t StateMachine::uvDutyFromIntensity() const {
    return mapIntensityToDuty(intensityPct_);
}

uint16_t StateMachine::displayedRemainingSec() const {
    if (remainingMs_ == 0) {
        return 0;
    }
    return static_cast<uint16_t>((remainingMs_ + 999UL) / 1000UL);
}


