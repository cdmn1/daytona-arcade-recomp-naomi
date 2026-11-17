# Naomi system and game test-menu requirements

Date: 2026-11-17

The VF3 Naomi project contains four user-provided reference screenshots in its `test menu flow/` folder. They define the expected Naomi operator flow for both the VF3 and Daytona Naomi ports.

## Reference flow

The Naomi **SYSTEM MENU** is entered with the cabinet service/test controls and contains platform-level functions:

- RAM TEST
- JVS TEST
- SOUND TEST
- C.R.T. TEST
- SYSTEM ASSIGNMENTS
- COIN ASSIGNMENTS
- BOOKKEEPING
- BACKUP DATA CLEAR
- CLOCK SETTING
- ROM BOARD TEST
- GAME TEST MODE
- EXIT

Selecting **GAME TEST MODE** loads the title's test program. The title's **GAME TEST MODE** contains:

- INPUT TEST
- OUTPUT TEST
- SOUND TEST
- GAME ASSIGNMENTS
- BOOKKEEPING
- BACKUP DATA CLEAR
- EXIT

Exiting the title test program returns to the Naomi system menu. The system menu and title test program are separate layers and must remain separate.

## Retail Naomi BIOS hand-off

Commercial Virtua Fighter 4 Final Tuned game and test-program code confirms
that both physical TEST during gameplay and EXIT from the title's test
program use retail Naomi BIOS helper entry 0x44 to transfer to the Naomi
System Test Menu.

The helper block is validated from BIOS ROM address 0xA0060000 and copied
(0x7000 bytes) to 0xAC018000. The function pointer at helper-table offset
0x44 is called through its uncached P2 alias. The service waits for G1 DMA
to become idle, acknowledges the cartridge bus, and does not return.

The reusable implementation is in platform/naomi/naomi_bios.c and
platform/naomi/naomi_bios.h. It exposes helper initialization, rising-edge
detection for the JVS TEST input, and the BIOS hand-off function. The caller
must stop title-specific work (network/card activity, audio, pending G1/DIMM
transfers, and Naomi 2 ELAN activity where applicable) before calling it.

Important: the current Daytona CMake project builds a host-native recompiled
game and has no SH-4/retail-Naomi executable target. Therefore this BIOS code
is present as the shared platform implementation for a future Naomi-native
target; it must not be linked into the current desktop executable. Once a
Naomi-native runtime exists, wire the same service into the game frame loop
and the separate title-test program's EXIT handler.

## Daytona implementation requirements

- Implement and validate the complete system-menu -> Daytona game-test program -> system-menu lifecycle.
- Service-button navigation and test-button selection must work through the actual Naomi operator controls.
- Each listed title-side item must open a functional test/settings page, or be explicitly documented as unsupported until implemented. A menu-shaped static screen is not sufficient.
- Input Test must show actual Naomi/JVS control state; Output Test must exercise supported cabinet outputs safely; Sound Test must exercise the title's audio path.
- Game Assignments must expose the Daytona-specific controls/options supported by the game. Do not assume VF3's title-specific assignments apply to Daytona.
- Bookkeeping and backup-data functions must use the title's real data structures. Destructive backup clearing requires an explicit confirmation step.
- Exit must return to the Naomi system menu cleanly without resetting or hanging the game/system.

## Validation

A Daytona test menu is complete only after the above lifecycle is exercised on Naomi hardware or a faithful Naomi system-menu environment, with working navigation, tests, assignments and return behavior. Previous test-menu work must not be assumed successful without this validation.

The VF3 reference screenshots are in the separate `cdmn1/vf3-naomi` repository under `test menu flow/`.
