# Daytona USA — Naomi port

This directory contains Naomi-specific integration and bring-up work for the
native SH-4 game in `platform/dreamcast-native`.

## Target and build environment

- Naomi 1, SH-4, KallistiOS Naomi sub-architecture.
- Set `KOS_SUBARCH=naomi` **before** sourcing the Naomi KOS environment.
- Use a KOS build that includes Naomi/NetDIMM support (KallistiOS 2.1.x or a
  later compatible revision).
- Keep the 256 MB NetDIMM as the minimum supported deployment target; the
  512 MB DIMM may be used for optional content.

Example environment setup (adjust the KOS path to the installed Naomi tree):

```sh
export KOS_SUBARCH=naomi
source /opt/toolchains/dc/kos-naomi/environ.sh
```

The exact KOS path is installation-specific. Do not source a Dreamcast-only
KOS tree and assume that changing the variable afterwards retargets its
already-built libraries. The standalone input probe does not need enDjinn,
game assets, or AICAflow.

## First bring-up test

Build the standalone KOS input probe:

```sh
make -C platform/naomi/tests/input_probe clean all
```

The Makefile refuses to build unless the active environment explicitly has
`KOS_SUBARCH=naomi` and `KOS_BASE` set. The output is
`platform/naomi/tests/input_probe/naomi_input_probe.elf`. This is a Naomi ELF
diagnostic, not the final NetDIMM `.bin` image.

It reports:
- the Naomi MIE/JVS bridge and its mapped controller state;
- native JVS wheel, accelerator, brake and switch values;
- ordinary Maple controllers, independently of JVS.

The probe is deliberately independent of game assets, enDjinn and AICAflow.
It is a ROM-free test of KOS startup and input discovery. Its successful
compilation does not prove Flycast or physical-cabinet operation; those need
separate runs.

## Input integration

KOS exposes Naomi JVS through `MAPLE_FUNC_MIE`, not as an ordinary
`MAPLE_FUNC_CONTROLLER` device. enDjinn's pinned controller mapper originally
enumerated only ordinary controller devices. The patch in
`patches/endjinn-mie-input.patch` makes the MIE device's already-mapped
`mie_state_t.cont` visible through enDjinn's existing normalized controller
state slot. Standard Maple gamepads remain supported.

The patch is applied by
`platform/dreamcast-native/scripts/fetch_endjinn.sh` after checking out the
pinned enDjinn revision. It is kept in this repository so the dependency
modification is reviewable and repeatable.

The game still needs a Daytona-specific JVS mapping decision for wheel,
accelerator, brake, start, service and coin inputs. The KOS default MIE mapper
already converts wheel and pedals to `cont_state_t`; this is a compatibility
starting point, not yet a claim of arcade-correct calibration.

## NetDIMM and game assets

A NetDIMM is treated as the resident disc-image source: the game image is
uploaded to DIMM RAM and the Naomi executes from that image. Do not treat DIMM
RAM as Naomi main RAM, and do not introduce a PC share or a separate asset
server as a runtime dependency.

KOS's documented `/cd` ISO9660 filesystem is Dreamcast-only. Therefore the
Dreamcast port's `NATIVE_DISC=1` and `/cd/` path is **not** assumed to work on
Naomi. The next storage milestone is to identify and test the actual Naomi
NetDIMM image access interface, then provide a game asset VFS backend that
reads the self-contained image while keeping the existing bounded scene and
audio loaders.

KOS also supplies `naomibintool`, which wraps a Naomi-targeted SH-4 ELF in
the Naomi ROM header format to produce the uploadable `.bin`. Its current
implementation packs one ELF image as one load segment; it is a ROM-image
builder, not a general-purpose asset filesystem or a way to map a large game
archive into Naomi main RAM.

The game still needs a runtime reader for the resident image on the Naomi ROM
board / NetDIMM path (including the appropriate G1/ROM-board DMA or PIO
mechanism). That interface must be verified against KOS and hardware/emulator
behaviour before the game asset VFS is written. The final packaging step will
use the KOS Naomi ROM format and then the user's existing NetDIMM uploader.

No Naomi asset backend is selected yet. Do not silently fall back to
`/pc/`, `/cd/`, network filesystems, or a host-mounted directory.

## Current status

- [x] Naomi KOS target confirmed in KOS documentation/source.
- [x] KOS MIE/JVS API and standard Maple controller API identified and checked against current KOS headers.
- [x] Input probe Makefile rejects non-Naomi KOS environments.
- [x] enDjinn MIE-to-controller compatibility patch prepared.
- [x] ROM-free input probe source and Makefile added.
- [ ] Build probe with a Naomi KOS toolchain (not yet compiled in this environment).
- [ ] Run probe in Flycast.
- [ ] Run probe on Naomi 1 with JVS cabinet and with a standard Maple controller.
- [ ] Verify NetDIMM resident-image read access.
- [ ] Add Naomi product build and validate PVR/AICA startup.
- [ ] Integrate fixed 57.52 Hz simulation cadence and verify on hardware.
- [ ] Implement Naomi save/settings backend (or explicitly disable saving initially).

No Naomi ELF, BIN, DAT or NetDIMM upload has been produced by this commit.
