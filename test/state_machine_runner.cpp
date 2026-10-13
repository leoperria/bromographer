#include <cassert>
#include <cstdio>

#include "config.h"
#include "events.h"
#include "state_machine.h"

namespace {

void driveToReady(StateMachine& sm, uint32_t& nowMs) {
    nowMs = 0;
    sm.begin(nowMs, {150, 80});
    sm.tick(500);
    assert(sm.state() == MachineState::Boot);
    nowMs = Config::kBootSplashMs;
    sm.tick(nowMs);
    assert(sm.state() == MachineState::Ready);
}

void testStartOnButtonRelease() {
    StateMachine sm;
    uint32_t now = 0;
    driveToReady(sm, now);

    sm.handleEvent({InputEventType::ButtonDown, 0, 1100}, 1100);
    assert(sm.state() == MachineState::Ready);

    sm.handleEvent({InputEventType::ButtonUp, 0, 1150}, 1150);
    assert(sm.state() == MachineState::Exposing);
}

void testPauseRequiresSecondTapToResume() {
    StateMachine sm;
    uint32_t now = 0;
    driveToReady(sm, now);

    sm.handleEvent({InputEventType::ButtonDown, 0, 1100}, 1100);
    sm.handleEvent({InputEventType::ButtonUp, 0, 1120}, 1120);
    assert(sm.state() == MachineState::Exposing);

    sm.tick(2000);

    sm.handleEvent({InputEventType::ButtonDown, 0, 2100}, 2100);
    assert(sm.state() == MachineState::Paused);
    assert(sm.outputs().ledMode == LedMode::VeryFastBlink);
    const uint32_t pausedRemaining = sm.remainingMs();

    sm.handleEvent({InputEventType::ButtonUp, 0, 2150}, 2150);
    assert(sm.state() == MachineState::Paused);
    assert(sm.remainingMs() == pausedRemaining);

    sm.handleEvent({InputEventType::ButtonDown, 0, 2300}, 2300);
    sm.handleEvent({InputEventType::ButtonUp, 0, 2350}, 2350);
    assert(sm.state() == MachineState::Exposing);
}

void testHoldResetFromPaused() {
    StateMachine sm;
    uint32_t now = 0;
    driveToReady(sm, now);

    sm.handleEvent({InputEventType::ButtonDown, 0, 1100}, 1100);
    sm.handleEvent({InputEventType::ButtonUp, 0, 1150}, 1150);
    assert(sm.state() == MachineState::Exposing);

    sm.handleEvent({InputEventType::ButtonDown, 0, 2000}, 2000);
    assert(sm.state() == MachineState::Paused);

    sm.handleEvent({InputEventType::ButtonHold, 0, 4000}, 4000);
    assert(sm.state() == MachineState::Ready);

    sm.handleEvent({InputEventType::ButtonUp, 0, 4100}, 4100);
    assert(sm.state() == MachineState::Ready);
}

void testTimeSteppingGrid() {
    StateMachine sm;
    sm.begin(0, {59, 80});
    sm.tick(Config::kBootSplashMs);
    assert(sm.state() == MachineState::Ready);

    // Slow turns: 1-second steps.
    assert(sm.currentSettings().timeSec == 59);
    sm.handleEvent({InputEventType::EncoderRotate, +1, 1210}, 1210);
    assert(sm.currentSettings().timeSec == 60);
    sm.handleEvent(
        {InputEventType::EncoderRotate, +1, static_cast<uint32_t>(1210 + Config::kEncoderFastTurnThresholdMs + 1)},
        static_cast<uint32_t>(1210 + Config::kEncoderFastTurnThresholdMs + 1));
    assert(sm.currentSettings().timeSec == 61);

    // Fast turns: 10-second steps.
    sm.handleEvent({InputEventType::EncoderRotate, +1, 2000}, 2000);
    assert(sm.currentSettings().timeSec == 62);
    sm.handleEvent(
        {InputEventType::EncoderRotate, +1, static_cast<uint32_t>(2000 + Config::kEncoderFastTurnThresholdMs)},
        static_cast<uint32_t>(2000 + Config::kEncoderFastTurnThresholdMs));
    assert(sm.currentSettings().timeSec == 72);

    sm.handleEvent(
        {InputEventType::EncoderRotate, -1, static_cast<uint32_t>(2000 + 2 * Config::kEncoderFastTurnThresholdMs)},
        static_cast<uint32_t>(2000 + 2 * Config::kEncoderFastTurnThresholdMs));
    assert(sm.currentSettings().timeSec == 62);
}

void testDoneAndConfirm() {
    StateMachine sm;
    uint32_t now = 0;
    driveToReady(sm, now);

    sm.handleEvent({InputEventType::ButtonDown, 0, 1100}, 1100);
    sm.handleEvent({InputEventType::ButtonUp, 0, 1150}, 1150);
    assert(sm.state() == MachineState::Exposing);

    // Run out timer.
    sm.tick(200000);
    assert(sm.state() == MachineState::Done);

    sm.handleEvent({InputEventType::EncoderPress, 0, 200100}, 200100);
    assert(sm.state() == MachineState::Ready);
}

void testLedModesForExposingAndDone() {
    StateMachine sm;
    uint32_t now = 0;
    driveToReady(sm, now);

    sm.handleEvent({InputEventType::ButtonDown, 0, 1100}, 1100);
    sm.handleEvent({InputEventType::ButtonUp, 0, 1150}, 1150);
    assert(sm.state() == MachineState::Exposing);
    assert(sm.outputs().ledMode == LedMode::Solid);

    sm.tick(200000);
    assert(sm.state() == MachineState::Done);
    assert(sm.outputs().ledMode == LedMode::FastBlink);
}

}  // namespace

int main() {
    testStartOnButtonRelease();
    testPauseRequiresSecondTapToResume();
    testHoldResetFromPaused();
    testTimeSteppingGrid();
    testDoneAndConfirm();
    testLedModesForExposingAndDone();

    std::puts("state_machine_runner: all tests passed");
    return 0;
}



