#include <Arduino.h>
#include <avr/wdt.h>

#include "config.h"
#include "display.h"
#include "input.h"
#include "outputs.h"
#include "state_machine.h"
#include "storage.h"
#include "timebase.h"

// On ATmega32U4, watchdog can survive reset; clear it before Arduino init.
void clearWatchdogAtBoot() __attribute__((naked)) __attribute__((section(".init3")));
void clearWatchdogAtBoot() {
    MCUSR = 0;
    wdt_disable();
}

namespace {

Display gDisplay;
Input gInput;
Outputs gOutputs;
Storage gStorage;
StateMachine gStateMachine;

MachineState gPrevState = MachineState::Boot;

constexpr bool kEnableWatchdogByDefault = true;
// Development override: set false to force watchdog off regardless of defaults.
constexpr bool kWatchdogOverrideEnabled = false;
bool gWatchdogEnabled = false;

void configureInputs() {
    pinMode(Config::kStartStopButtonPin, INPUT_PULLUP);
    pinMode(Config::kEncoderKeyPin, INPUT_PULLUP);
    pinMode(Config::kEncoderS1Pin, INPUT_PULLUP);
    pinMode(Config::kEncoderS2Pin, INPUT_PULLUP);
}

}  // namespace

void setup() {
    // Safety first: force UV output off before any slower initialization work.
    Outputs::forceUvOffEarly();

    configureInputs();

    Serial.begin(115200);

    const uint32_t initMs = Timebase::nowMs();

    gOutputs.begin(initMs);
    gDisplay.begin();
    gInput.begin(initMs);

    const PersistedSettings settings = gStorage.loadOrDefault();
    const uint32_t bootStartMs = Timebase::nowMs();
    gStateMachine.begin(bootStartMs, settings);
    gPrevState = gStateMachine.state();

    // Render boot screen immediately so it is visible even before first loop tick.
    gDisplay.render(gStateMachine.makeScreen(bootStartMs));
    gOutputs.apply(gStateMachine.outputs(), bootStartMs);

    gWatchdogEnabled = kEnableWatchdogByDefault && kWatchdogOverrideEnabled;

    if (gWatchdogEnabled) {
        wdt_enable(WDTO_8S);
    }

    Serial.print("WDT: ");
    Serial.println(gWatchdogEnabled ? "ON (8s)" : "OFF (override)");
}

void loop() {
    if (gWatchdogEnabled) {
        wdt_reset();
    }

    const uint32_t nowMs = Timebase::nowMs();

    gInput.poll(nowMs);

    InputEvent event{};
    while (gInput.pop(&event)) {
        gStateMachine.handleEvent(event, nowMs);
        gOutputs.apply(gStateMachine.outputs(), nowMs);
    }

    gStateMachine.tick(nowMs);

    const MachineState stateNow = gStateMachine.state();
    if (gPrevState != MachineState::Exposing && stateNow == MachineState::Exposing) {
        gOutputs.triggerStartBuzz(nowMs);
        gStorage.saveIfChanged(gStateMachine.currentSettings());
    }
    if (gPrevState != MachineState::Done && stateNow == MachineState::Done) {
        gOutputs.startDoneBuzz(nowMs);
    }
    if (gPrevState == MachineState::Done && stateNow != MachineState::Done) {
        gOutputs.stopBuzz();
    }
    gPrevState = stateNow;

    gDisplay.render(gStateMachine.makeScreen(nowMs));
    gOutputs.apply(gStateMachine.outputs(), nowMs);
}