# Naomi input mapping

## KOS input sources

KOS exposes two independent Maple function types:

- `MAPLE_FUNC_MIE`: Naomi's MIE/JVS bridge. Its status is `mie_state_t`,
  containing native JVS values (`jvs`) and a mapped `cont_state_t`
  (`cont`).
- `MAPLE_FUNC_CONTROLLER`: ordinary Maple controllers (including compatible
  gamepads when exposed by the platform/emulator).

The Naomi patch in `patches/endjinn-mie-input.patch` enumerates MIE devices
separately and, when no ordinary controller occupies that port, passes
`mie_state_t.cont` through enDjinn's existing
`enj_ctrl_kos2enj_state()` conversion. This preserves the game's normal
button edge tracking and controller-state consumers. Standard Maple
controllers remain supported.

## Daytona's existing normalized input path

The race loop in `platform/dreamcast-native/code/practice.c` already consumes
enDjinn's normalized controller state:

- `joyx` feeds the existing steering curve and ADC conversion.
- `rtrigger` feeds accelerator input.
- `ltrigger` feeds brake input.
- normalized START/A/B/X/Y and directional buttons feed start, view,
  transmission and menu actions.

KOS's MIE mapper exposes the JVS wheel and pedals through those same normalized
fields. Therefore the current integration can reuse Daytona's existing
control path without adding a separate game-specific polling loop.

## Native JVS data

The native JVS state is also available from `mie_state_t.jvs` for later
cabinet-specific handling:

- `wheel`, `accel`, and `brake`
- `p1` and `p2` switch banks
- `panel` for test/service/DIP switches
- `coin[]` for coin meters and status

KOS provides `mie_analog_norm_wheel()`, `mie_analog_norm_accel()` and
`mie_analog_norm_brake()` for calibrated analog values. Do not bypass the
existing Daytona steering curve or assume every JVS I/O board has identical
calibration without testing the target cabinet.

## Validation status

The v5 Naomi input probe has been run successfully in Flycast. It reports
`MIE JVS Bridge`, function mask `00000001`, and a readable mapped
`cont_state_t` plus native wheel, accelerator, brake, switch and coin fields.
The user also confirmed that button inputs respond in the probe.

This validates MIE discovery and the KOS state-reading path in Flycast. It
does not yet validate the full game's input handling, analog calibration, or
physical Naomi cabinet operation.

## Remaining Daytona-specific checks

- Verify the in-game steering, accelerator and brake response in Flycast.
- Verify start, view, gear and menu buttons in the game.
- Verify coin/service/test behavior separately before wiring those native
  JVS fields into game logic.
- Test with a physical Naomi/JVS cabinet before claiming cabinet calibration.
