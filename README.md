# Bromographer UV Exposure Timer

Firmware for a UV exposure timer on SparkFun Pro Micro (ATmega32U4), using a 16x2 I2C LCD, rotary encoder, and start/stop button.

## Hardware map (fixed)

- UV PWM output (MOSFET gate): `D5`
- Status LED: `D4`
- Start/stop button (active-low, pull-up): `D6`
- Encoder A/B: `D7` / `D8`
- Encoder push (active-low, pull-up): `D9`
- LCD: I2C `0x27`, 16x2

## Implemented behavior

- States: `Boot`, `Ready`, `Exposing`, `Paused`, `Done`
- Main button events: `DOWN`, `HOLD` (2s), `UP`
- Encoder rotation controls selected field only in `Ready`
- Encoder push toggles field in `Ready`; long push (>1s) ignored
- Settings are locked during `Exposing` and `Paused` with on-screen hint
- UV output is on only in `Exposing`
- Status LED patterns:
  - `Off`: Boot/Ready
  - `800ms on / 800ms off`: Exposing
  - `100ms on / 100ms off`: Paused
  - `200ms on / 200ms off`: Done

## PWM configuration

`D5` is configured as Timer3 OC3A, 8-bit fast PWM, prescaler 1.

- Expected PWM frequency: `16 MHz / 256 = 62.5 kHz`
- Duty cycle mapping: linear (`intensity% -> 0..255`)

## Persistence

- Time/intensity saved only on entering `Exposing`, and only if changed
- Values loaded and validated on boot; invalid/blank values fall back to defaults

## Watchdog behavior

- Runtime watchdog is enabled with an `8s` timeout.
- If the start/stop button (`D6`) is held during power-up/reset, firmware enters safe boot and leaves watchdog disabled for that boot.
- Early startup code clears/disables any inherited watchdog state before normal initialization (ATmega32U4 upload safety).

## Build and flash

```bash
cd /home/leonardo/repos/bromographer
platformio run
platformio run --target upload
```

## Run desktop state-machine tests

```bash
cd /home/leonardo/repos/bromographer
g++ -std=c++17 -Iinclude test/state_machine_runner.cpp src/state_machine.cpp -o /tmp/state_machine_runner
/tmp/state_machine_runner
```

## Open decisions currently fixed by default

- Linear intensity mapping (kept as requested)
- I2C LCD interface
- Active-low switches with internal pull-ups
- Encoder pulses-per-detent default: `4`

