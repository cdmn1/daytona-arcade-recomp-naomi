# Naomi input mapping

## KOS input sources

KOS exposes two independent Maple function types:

- `MAPLE_FUNC_MIE`: Naomi's MIE/JVS bridge. Its status is `mie_state_t`,
  containing both native JVS values (`jvs`) and a mapped
  `cont_state_t` (`cont`).
- `MAPLE_FUNC_CONTROLLER`: ordinary Maple controllers (including compatible
  gamepads when exposed by the platform/emulator).

The enDjinn patch keeps the existing four controller slots and maps the MIE
device into the slot for its Maple port when there is no ordinary controller
device in that slot. It passes `mie_state_t.cont` through the same
`enj_ctrl_kos2enj_state()` conversion as a standard controller. This preserves
button edge tracking and the game's existing input consumers.

## Native JVS values

The native JVS state remains available separately for cabinet-specific
handling:

- `mie_state_t.jvs.wheel`
- `mie_state_t.jvs.accel`
- `mie_state_t.jvs.brake`
- `mie_state_t.jvs.p1` and `p2` switch banks
- `mie_state_t.jvs.panel` for test/service/DIP switches
- `mie_state_t.jvs.coin[]` for coin meters and status

The KOS default mapper maps the wheel to `cont.joyx`, accelerator to
`cont.rtrig`, brake to `cont.ltrig`, and common JVS buttons to the familiar
Dreamcast controller buttons. KOS also provides
`mie_analog_norm_wheel()`, `mie_analog_norm_accel()` and
`mie_analog_norm_brake()` for calibrated analog values.

## Daytona control policy (to be finalized on cabinet)

- Steering: use native calibrated JVS wheel data when MIE is present; preserve
  the existing steering curve/ADC conversion after normalization.
- Accelerator and brake: use native calibrated JVS pedal values; preserve
  the existing arcade control decoder.
- Start, view, gear and menu actions: use the shared normalized button state.
- Coin, service and test: consume native JVS fields, not simulated pad buttons.
- Standard gamepad: retain the current enDjinn mapping and its analog triggers.

Do not assume every JVS I/O board uses the same analog channel assignment or
calibration range. Confirm the actual cabinet's reported values with the input
probe before setting game defaults.
