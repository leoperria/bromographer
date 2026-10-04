## Plan: UV Timer Firmware Refactor

Use the approved UX/state spec as the source of truth, while preserving your already-validated hardware wiring exactly (`D5` UV PWM, `D4` status LED, `D6` button, encoder/display pins and active-low inputs). Implement a modular non-blocking architecture, keep linear PWM mapping, and configure UV PWM on Pro Micro (`ATmega32U4`) to the highest practical hardware frequency available on the existing `D5` timer path, without changing pins.

### Steps
1. Freeze fixed hardware constants in [src/main.cpp](/home/leonardo/repos/bromographer/src/main.cpp) and new `Config` symbols in [include/config.h](/home/leonardo/repos/bromographer/include/config.h).
2. Split firmware into modules under [src/](/home/leonardo/repos/bromographer/src): `timebase`, `input`, `state_machine`, `display`, `outputs`, `storage`.
3. Implement pure `StateMachine` from spec sections 5–6 with explicit events and `MachineState` transitions.
4. Build input pipeline for button `DOWN/HOLD/UP`, encoder detents, and encoder-push long-press ignore using queueing.
5. Implement LCD renderer for exact 16x2 layouts, CGRAM bar/arrow glyphs, and diff-only writes in `Display`.
6. Integrate outputs and persistence: linear UV mapping, max-frequency PWM on `D5`, LED patterns, save-on-enter-Exposing, validated boot load.

### Further Considerations
1. PWM frequency on `D5` depends on `ATmega32U4` timer routing; keep `D5` fixed and select highest stable timer mode/prescaler available.
2. Add a `README` section documenting chosen timer register setup and resulting measured PWM frequency.
3. Draft for your review: proceed phase-by-phase against the acceptance checklist before optional enhancements.

